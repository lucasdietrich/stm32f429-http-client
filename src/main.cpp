#include <zephyr.h>
#include <linker/sections.h>
#include <errno.h>
#include <stdio.h>

#include <net/net_if.h>
#include <net/net_core.h>
#include <net/net_context.h>
#include <net/net_mgmt.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(net_dhcpv4_client_sample, LOG_LEVEL_DBG);



void main(void)
{
    LOG_INF("Starting [%d]", errno);
    while (true)
    {
        k_sleep(K_MSEC(1000));

        printk("UU");
    }
}
