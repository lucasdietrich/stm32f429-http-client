#include "net.h"

#include <net/net_if.h>
#include <net/net_core.h>
#include <net/net_context.h>
#include <net/net_mgmt.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(app_net, LOG_LEVEL_DBG);

#include "hw.h"

/*___________________________________________________________________________*/

static struct net_mgmt_event_callback mgmt_cb[2];

struct k_poll_signal signal_net_addr_up; // addr up

/*___________________________________________________________________________*/

static void net_event_handler(struct net_mgmt_event_callback *cb,
		    uint32_t mgmt_event,
		    struct net_if *iface)
{
    switch (mgmt_event)
    {

    // if lost its IP address
    case NET_EVENT_IPV4_ADDR_DEL:
        hw_set_led(GREEN, OFF);
        _LOG_WRN("~ NET_EVENT_IPV4_ADDR_DEL");
        break;


    // fun : pluging the RJ45 Ethernet cable trigger this interrupt
    case NET_EVENT_IF_UP:
        hw_set_led(RED, OFF);
        hw_set_led(GREEN, ON);
        LOG_DBG("~ NET_EVENT_IF_UP");
        break;

    // fun : unpluging the RJ45 Ethernet cable trigger this interrupt
    case NET_EVENT_IF_DOWN:
        hw_set_led(GREEN, OFF);
        hw_set_led(RED, ON);
        _LOG_WRN("~ NET_EVENT_IF_DOWN");
        break;

    // client DHCP succeeded, the if get an IP address
    case NET_EVENT_IPV4_ADDR_ADD:
        hw_set_led(GREEN, ON);

        LOG_DBG("~ NET_EVENT_IPV4_ADDR_ADD");

        // display address
        for (uint_fast16_t i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) // short to 1 here (1 interface)
        {
            char buf[NET_IPV4_ADDR_LEN];

            if (iface->config.ip.ipv4->unicast[i].addr_type != NET_ADDR_DHCP)
            {
                //TODO
                _LOG_INF("\n NET_ADDR_DHCP ~ continue");
                continue;
            }

            _LOG_INF("IP address received from DHCP server");

            LOG_INF("Your address: %s",
                    log_strdup(net_addr_ntop(AF_INET,
                                             &iface->config.ip.ipv4->unicast[i].address.in_addr,
                                             buf, sizeof(buf))));
            LOG_DBG("Lease time: %u seconds",
                    iface->config.dhcpv4.lease_time);
            LOG_DBG("Subnet: %s",
                    log_strdup(net_addr_ntop(AF_INET,
                                             &iface->config.ip.ipv4->netmask,
                                             buf, sizeof(buf))));
            LOG_DBG("Router: %s",
                    log_strdup(net_addr_ntop(AF_INET,
                                             &iface->config.ip.ipv4->gw,
                                             buf, sizeof(buf))));
        }

        k_poll_signal_raise(&signal_net_addr_up, 1);

        break;

    default:
        break;
    }
}


void init_if(void)
{
    k_poll_signal_init(&signal_net_addr_up);

    struct k_poll_event events[1] =
    {
        K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &signal_net_addr_up)
    };

    // IF
    net_mgmt_init_event_callback(&mgmt_cb[0], net_event_handler, NET_EVENT_IF_UP | NET_EVENT_IF_DOWN);
	net_mgmt_add_event_callback(&mgmt_cb[0]);

    // DHCP
	net_mgmt_init_event_callback(&mgmt_cb[1], net_event_handler, NET_EVENT_IPV4_ADDR_ADD | NET_EVENT_IPV4_ADDR_DEL);
	net_mgmt_add_event_callback(&mgmt_cb[1]);

    struct net_if *const iface = net_if_get_default();
    net_dhcpv4_start(iface);

    // waiting to net *if* to have an ip address before returning
    k_poll(events, 1, K_FOREVER);
}