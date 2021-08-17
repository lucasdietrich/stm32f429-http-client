#ifndef _HW_H
#define _HW_H

#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>

/*___________________________________________________________________________*/

#define LED_GREEN_NODE DT_ALIAS(led0)

// GREEN LED
#if DT_NODE_HAS_STATUS(LED_GREEN_NODE, okay)
#define LED_GREEN_PORT	        DT_GPIO_LABEL(LED_GREEN_NODE, gpios)
#define LED_GREEN_PIN	        DT_GPIO_PIN(LED_GREEN_NODE, gpios)
#define LED_GREEN_FLAGS	        DT_GPIO_FLAGS(LED_GREEN_NODE, gpios)
#else

/* A build error here means your board isn't set up to blink an LED. */
#error "Unsupported board: led0 devicetree alias is not defined"
#endif

#define LED_BLUE_NODE DT_ALIAS(led1)

// BLUE LED
#if DT_NODE_HAS_STATUS(LED_BLUE_NODE, okay)
#define LED_BLUE_PORT           DT_GPIO_LABEL(LED_BLUE_NODE, gpios)
#define LED_BLUE_PIN	        DT_GPIO_PIN(LED_BLUE_NODE, gpios)
#define LED_BLUE_FLAGS	        DT_GPIO_FLAGS(LED_BLUE_NODE, gpios)
#else

/* A build error here means your board isn't set up to blink an LED. */
#error "Unsupported board: led1 devicetree alias is not defined"
#endif

#define LED_RED_NODE DT_ALIAS(led2)

// RED LED
#if DT_NODE_HAS_STATUS(LED_RED_NODE, okay)
#define LED_RED_PORT	         DT_GPIO_LABEL(LED_RED_NODE, gpios)
#define LED_RED_PIN	            DT_GPIO_PIN(LED_RED_NODE, gpios)
#define LED_RED_FLAGS	        DT_GPIO_FLAGS(LED_RED_NODE, gpios)
#else

/* A build error here means your board isn't set up to blink an LED. */
#error "Unsupported board: led2 devicetree alias is not defined"
#endif

/*___________________________________________________________________________*/

enum led_e { GREEN, BLUE, RED };

enum led_state_t { OFF, ON };

struct led_t {
    const struct device *dev;
    gpio_pin_t pin;
};

/*___________________________________________________________________________*/

/**
 * @brief Initialize leds (green, blue, red) and blink them during 1 second
 */
void hw_init_leds(void);

void hw_set_led(led_e led, led_state_t state);

void hw_set_led(led_t *led, led_state_t state);


#endif