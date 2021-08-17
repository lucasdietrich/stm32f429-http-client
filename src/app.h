#include "header.h"

#include "hw.h"
#include "net.h"

#include <net/net_core.h>
#include <net/net_context.h>
#include <net/http_client.h>
#include <net/net_ip.h>
#include <net/dns_resolve.h>
#include <net/socket.h>

#define HTTP_SERVER_PORT 8080

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
        idle = 1, // initialized
        server_resolve = 2,    // ip addresse resolvedd
        token_receive = 3,     // provisionning done
        server_connect = 4,          // connected    -> http request with token saying connected
        connected = 5,
        disconnected = 6,       // disconnected -> http request with token saying disconnected
    };

    static const char * const get_app_state_str(app_state state);

/*___________________________________________________________________________*/

    const char * const m_server_hostname = "pcluc.local.lan";

    struct in_addr m_server_ipaddr = { .s_addr = 0u };

    uint8_t m_token[32];

    app_state m_state = undefined;

    int m_client_socket = -1;


/*___________________________________________________________________________*/

    void state_machine(void);

    int resolve_server(const char * const hostname);

    int setup_socket(void);

    int obtain_token(void);

    // check if token is already set
    bool registered(void);

    void reset(void);

/*___________________________________________________________________________*/

};
