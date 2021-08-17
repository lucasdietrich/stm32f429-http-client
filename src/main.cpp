#include <zephyr.h>
#include <linker/sections.h>
#include <errno.h>
#include <stdio.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#include "header.h"
#include "app.h"

void main(void)
{
    Application app = Application();

    app.init();
    
    while(1)
    {
        app.state_machine();
    }
}
