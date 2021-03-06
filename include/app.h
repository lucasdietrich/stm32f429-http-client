#include "header.h"

#include <net/net_ip.h>
#include <net/http_client.h>
#include <net/dns_resolve.h>
#include <data/json.h>

#define HTTP_SERVER_PORT    8080

#define URL_GET_TOKEN           "/getToken?Seed="
#define SEED                    "258e3f3bd8a4a54aabcf5ef6723effdd"
#define URL_GET_TOKEN_SIZE      sizeof(URL_GET_TOKEN SEED)
#define TOKEN_SIZE              sizeof("5f52de492032c5be3f3467f7cfc0081dd60fb40d")

class Application
{
public:

/*___________________________________________________________________________*/

    static Application * p_instance;
    Application() { p_instance = this; }
    static Application * const get_instance(void) { return p_instance; }

    void init(void);

/*___________________________________________________________________________*/
    
    enum app_state
    {
        undefined = 0,
        idle = 1,                    // initialized
        server_resolve = 2,          // ip addresse resolvedd
        token_receive = 3,           // provisionning
        server_connect = 4,          // provisionning finished or already done, connecting http request with token saying connected
        connected = 5,               // connection
        disconnecting = 6,              // disconnected -> http request with token saying disconnected
    };

    static const char * const get_app_state_str(app_state state);

/*___________________________________________________________________________*/

    app_state m_state = undefined;

    const char * const m_server_hostname = "pcluc.local.lan";
    int m_client_socket = -1;
    struct in_addr m_server_ipaddr = { .s_addr = 0u };

    uint32_t m_seed[4] = {};
    char m_token[TOKEN_SIZE] = {};   // sha1 is 40 byte long

    char m_get_token_url[URL_GET_TOKEN_SIZE];
    struct http_request m_request;

    static uint8_t m_tmp_buffer[256];

    static uint8_t *m_buffer_cursor;
    static uint8_t m_buffer[2048];

/*___________________________________________________________________________*/

    static void dns_result_cb(enum dns_resolve_status status, struct dns_addrinfo *info, void *user_data);

    static int http_response_body_cb(struct http_parser *parser, const char *at, size_t length);
    
    static void http_response_cb(struct http_response *rsp, enum http_final_call final_data, void *user_data);

/*___________________________________________________________________________*/

    void state_machine(void);

    void set_state(enum app_state new_state);

/*___________________________________________________________________________*/

    int resolve_server(const char * const hostname);

    int setup_socket(void);

    void generate_seed(void);

    void build_token_url(void);

    void setup_http_request(const char *const url);

    int obtain_token(void);

    // check if token is already set
    bool is_registered(void);

    void reset(bool force_provisionning = false);

/*___________________________________________________________________________*/

};
