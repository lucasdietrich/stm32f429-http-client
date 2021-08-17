#include "app.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(app, LOG_LEVEL_DBG);

/*___________________________________________________________________________*/

Application * Application::p_instance;

const static char *const app_state_str[] = {
    "undefined",
    "idle",
    "server_resolved",
    "token_received",
    "connected",
    "disconnected"
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
    LOG_INF("state_machine state: %s", get_app_state_str(m_state));

    switch (m_state)
    {
    case idle:
        m_state = server_resolve;
        break;

    case server_resolve:
        if ((m_server_ipaddr.s_addr != 0) || (0 == resolve_server(m_server_hostname)))
        {
            m_state = server_connect;

        }
        break;

    case server_connect:
        if (setup_socket() >= 0)
        {
            if (registered())
            {
                m_state = connected;
            }
            else
            {
                m_state = token_receive;
            }            
        }
        else
        {
            // reset and go idle
            reset();
        }
        break;

    case token_receive:
        if (0 == obtain_token())
        {
            m_state = connected;
        }
        break;

    case connected: // ready
        m_state = disconnected;
        break;

    case disconnected:
        m_state = idle;
        break;
    
    default:
        _LOG_ERR("undefined state");
    }

    k_sleep(K_MSEC(5000));
}

/*___________________________________________________________________________*/

void Application::reset(void)
{
    if(m_client_socket >= 0)
    {
        close(m_client_socket);
    }

    memset(m_token, 0x00, sizeof(m_token));
    // m_server_ipaddr.s_addr = 0u;
    m_state = idle;

    _LOG_ERR("RESET");
}

/*___________________________________________________________________________*/

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

        // set secure

        ret = connect(m_client_socket, (const sockaddr *) &addr, sizeof(struct sockaddr_in));

        if (ret != 0)
        {
            _LOG_ERR("failed to connect to server");
        }

        close(m_client_socket);

        reset();
    }
    else
    {
        LOG_ERR("failed to create HTTP socket : %d", m_client_socket);
    }
    return ret;
}

int Application::obtain_token(void)
{
    return 0;
}

/*___________________________________________________________________________*/

static void dns_result_cb(enum dns_resolve_status status, struct dns_addrinfo *info, void *user_data)
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

    memcpy(&Application::get_instance()->m_server_ipaddr, addr, sizeof(sockaddr_in::sin_addr));
    
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

bool Application::registered(void)
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