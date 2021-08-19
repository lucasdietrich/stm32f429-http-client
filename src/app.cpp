#include "app.h"

#include <net/net_core.h>
#include <net/net_context.h>
#include <net/http_client.h>
#include <net/socket.h>
#include <random/rand32.h>

#include "hw.h"
#include "net.h"
#include "app_utils.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(app, LOG_LEVEL_DBG);

/*___________________________________________________________________________*/

Application * Application::p_instance;

uint8_t Application::m_tmp_buffer[256];

uint8_t *Application::m_buffer_cursor;
uint8_t Application::m_buffer[2048];

const static char *const app_state_str[] = {
    "undefined",
    "idle",
    "server_resolve",
    "token_receive",
    "server_connect",
    "connected",
    "disconnecting"
};

const char *const Application::get_app_state_str(app_state state)
{
    if (state >= ARRAY_SIZE(app_state_str))
    {
        state = undefined;
    }
    return app_state_str[state];
}

/*___________________________________________________________________________*/

struct k_poll_signal signal;

void Application::init(void)
{
    hw_init_leds();
    init_if();

    k_poll_signal_init(&signal);

    m_state = idle;
}

void Application::state_machine(void)
{
    switch (m_state)
    {
    case idle:
        set_state(server_resolve);
        break;

    case server_resolve:
        if ((m_server_ipaddr.s_addr != 0) || (0 == resolve_server(m_server_hostname)))
        {
            set_state(server_connect);
        }
        break;

    case server_connect:
        if (setup_socket() >= 0)
        {
            set_state(is_registered() ? connected : token_receive);       
        }
        else
        {
            // reset and go idle
            reset();
        }
        break;

    case token_receive:
        generate_seed();
        build_token_url();
        if (0 == obtain_token())
        {
            set_state(connected);
        }
        break;

    case connected:
        k_sleep(K_MSEC(10000));
        set_state(disconnecting);
        break;

    case disconnecting:
        reset();
        break;
    
    default:
        _LOG_ERR("undefined state");
    }
}

void Application::set_state(enum app_state new_state)
{
    if (new_state != m_state)
    {
        LOG_INF("[%s > %s]", log_strdup(get_app_state_str(m_state)), log_strdup(get_app_state_str(new_state)));
        m_state = new_state;
    }
}

/*___________________________________________________________________________*/

void Application::reset(bool force_provisionning)
{
    if(m_client_socket >= 0)
    {
        close(m_client_socket);
        m_client_socket = -1;
    }

    // don't reset token and ip
    // memset(m_token, 0x00, sizeof(m_token));
    // m_server_ipaddr.s_addr = 0u;

    if (force_provisionning)
    {
        memset(m_token, 0x00, sizeof(m_token));
    }

    set_state(idle);

    _LOG_INF("RESET");
}

/*___________________________________________________________________________*/

//
// Socket
//

int Application::setup_socket(void)
{
    int ret;

    ret = socket(AF_INET, SOCK_STREAM, 0);
    if (ret >= 0)
    {
        m_client_socket = ret;

        struct sockaddr_in addr = {
            .sin_family = AF_INET,
            .sin_port = htons(HTTP_SERVER_PORT),
            .sin_addr = m_server_ipaddr
        };

        // TODO set secure

        ret = connect(m_client_socket, (const sockaddr *) &addr, sizeof(struct sockaddr_in));

        if (ret != 0)
        {
            LOG_ERR("failed to connect to server ret=%d", ret);
        }
    }
    else
    {
        LOG_ERR("failed to create HTTP socket : %d", m_client_socket);
    }
    return ret;
}

/*___________________________________________________________________________*/

//
// Token
//

void Application::generate_seed(void)
{
    // we generate a random hexadecimal number that will be send to the provisionning server
    sys_rand_get(m_seed, sizeof(m_seed));
}

void Application::build_token_url(void)
{
    strcpy(m_get_token_url, URL_GET_TOKEN);

    int ret = tohex(m_get_token_url + sizeof(URL_GET_TOKEN) - 1, sizeof(SEED), (uint8_t*) m_seed, sizeof(m_seed));
    m_get_token_url[URL_GET_TOKEN_SIZE - 1] = '\0';

    __ASSERT(ret > 0, "seed HEX encoding should never fail err=%d", ret);

    LOG_DBG("provisionning url: %s", log_strdup(m_get_token_url));
}

static const struct http_parser_settings http_settings_cb = {
    .on_body = Application::http_response_body_cb
};

void Application::setup_http_request(const char *const url)
{
    memset(&m_request, 0x00, sizeof(struct http_request));
    
    static const char * headers[] = {
        "Connection: close\r\n",
        NULL
    };

    m_request.method = http_method::HTTP_GET;
    m_request.response = http_response_cb;
    m_request.http_cb = &http_settings_cb;
    m_request.recv_buf = m_tmp_buffer;
    m_request.recv_buf_len = sizeof(m_tmp_buffer);
    m_request.url = url;
    m_request.protocol = "HTTP/1.1";
    // m_request.header_fields = headers; // settings Connection: Close error cause a bug

    // can be ignored ?
    m_request.host = m_server_hostname;
    m_request.port = "8080";
}

int Application::http_response_body_cb(struct http_parser *parser, const char *at, size_t length)
{
    if (m_buffer_cursor - m_buffer < (int) sizeof(m_buffer))
    {
        memcpy(m_buffer_cursor, at, length);
        m_buffer_cursor += length;
    }
    return 0;
}

struct http_token_response {
    const char * datetime;
    uint32_t timestamp;
    const char * seed;
    const char * salt;
    const char * token;
};

static const struct json_obj_descr http_token_response_desrc[] = {
    JSON_OBJ_DESCR_PRIM_NAMED(struct http_token_response, "Datetime", datetime, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM_NAMED(struct http_token_response, "Timestamp", timestamp, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM_NAMED(struct http_token_response, "Seed", seed, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM_NAMED(struct http_token_response, "Salt", salt, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM_NAMED(struct http_token_response, "Token", token, JSON_TOK_STRING),
};

void Application::http_response_cb(struct http_response *rsp, enum http_final_call final_data, void *user_data)
{
    if (final_data == HTTP_DATA_FINAL)
    {
        LOG_HEXDUMP_DBG(m_buffer, (size_t) (m_buffer_cursor - m_buffer), "");

        struct http_token_response data;

        int ret = json_obj_parse((char*) m_buffer, (size_t)(m_buffer_cursor - m_buffer), http_token_response_desrc,
                                 ARRAY_SIZE(http_token_response_desrc), &data);
        if (ret >= 0)
        {
            if (JSON_PARAM_FILLED(ret, 2))  // seed
            {
                // check seed
            }

            if (JSON_PARAM_FILLED(ret, 4))  // token
            {
                if (strlen(data.token) < sizeof(Application::m_token))
                {
                    strcpy((char *) p_instance->m_token, data.token);

                    // provisionning finished
                    p_instance->set_state(connected);
                }
                else
                {
                    _LOG_ERR("Token has invalid size");
                }
            }
            else
            {
                _LOG_ERR("Token not in response");
            }
        }
        else
        {
            _LOG_ERR("Invalid JSON response");
        }
    }
}

int Application::obtain_token(void)
{
    int ret = 0;

    LOG_DBG("m_seed=%s", log_strdup(m_get_token_url));

    setup_http_request(m_get_token_url);

    m_buffer_cursor = m_buffer;
    ret = http_client_req(m_client_socket, &m_request, 10000, nullptr); // &m_state
    if (ret < 0)
    {
        LOG_ERR("Failed to obtain token ret=%d", ret);
    }
    LOG_INF("Token: %s", log_strdup(m_token));
    return ret;
}

/*___________________________________________________________________________*/

void Application::dns_result_cb(enum dns_resolve_status status, struct dns_addrinfo *info, void *user_data)
{
	char hr_addr[NET_IPV4_ADDR_LEN];
	void *addr;

	switch (status) {
	case DNS_EAI_CANCELED:
		_LOG_INF("DNS query was canceled");
		return;
	case DNS_EAI_FAIL:
		_LOG_INF("DNS resolve failed");
		return;
	case DNS_EAI_NODATA:
		_LOG_INF("Cannot resolve address");
		return;
	case DNS_EAI_ALLDONE:
		_LOG_INF("DNS resolving finished");
		return;
	case DNS_EAI_INPROGRESS:
		break;
	default:
		LOG_INF("DNS resolving error (%d)", status);
		return;
	}

	if (!info) {
		return;
	}

	if (info->ai_family == AF_INET) {
		addr = &net_sin(&info->ai_addr)->sin_addr;
	} else {
		LOG_ERR("Invalid IP address family %d", info->ai_family);
		return;
	}

	LOG_INF("%s %s address: IPv4", user_data ? (char *)user_data : "<null>",
		log_strdup(net_addr_ntop(info->ai_family, addr,
					 hr_addr, sizeof(hr_addr))));

    memcpy(&p_instance->m_server_ipaddr, addr, sizeof(sockaddr_in::sin_addr));
    
    k_poll_signal_raise(&signal, 1);
}

int Application::resolve_server(const char * const hostname)
{
    uint16_t dns_id = 0x17;
    int ret = dns_get_addr_info(hostname, DNS_QUERY_TYPE_A, &dns_id, dns_result_cb, (void*) hostname, SYS_FOREVER_MS);

    LOG_DBG("dns_get_addr_info=%d", ret);

    if (ret == 0)
    {
        struct k_poll_event events[1] =
        {
            K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &signal)
        };

        ret = k_poll(events, 1, K_MSEC(5000));
        if (ret != 0)   // cancel the dns resolution, which took too long
        {
            dns_cancel_addr_info(dns_id);
        }
        return ret;
    }
    else
    {
        // failed to start dns resolution
    }
    return ret;
}

bool Application::is_registered(void)
{
    bool registered = false;
    for (uint_fast8_t i = 0; i < sizeof(m_token); i++)
    {
        if (m_token[i] != 0)
        {
            registered = true;
            break;
        }
    }
    return registered;
}

/*___________________________________________________________________________*/