# RGB LED

### Overview

Provides functions for controlling an RGB LED. This library implements color mixing and brightness control using the PWM blocks.

- The cy_rgb_led_init() function accepts the active logic (high or low) of the RGB LED as a parameter. All three (red, green, and blue) LEDs must be connected in same configuration, either active high or active low.
- The cy_rgb_led_init() function registers a low power handler that prevents the device from entering low power (deep-sleep) mode when the RGB LED is ON. You need to call cy_rgb_led_off() to turn the LED OFF before entering low power mode.

### Quick Start

1. Add \#include "cy_rgb_led.h"
2. Add the following code to the main() function. This example initializes the RGB LED and produces a yellow color with maximum brightness.

    CYBSP_LED_RGB_RED, CYBSP_LED_RGB_GREEN, and CYBSP_LED_RGB_BLUE are defined in the BSP.

```cpp
    cy_rslt_t result;

    result = cybsp_init();

    if(result == CY_RSLT_SUCCESS) {
        result = cy_rgb_led_init(CYBSP_LED_RGB_RED, CYBSP_LED_RGB_GREEN, CYBSP_LED_RGB_BLUE, CY_RGB_LED_ACTIVE_LOW);
        cy_rgb_led_on(CY_RGB_LED_COLOR_YELLOW, CY_RGB_LED_MAX_BRIGHTNESS);
    }

    for(;;) {
    }
```

### More information

* [API Reference Guide](https://cypresssemiconductorco.github.io/rgb-led/html/index.html)
* [Cypress Semiconductor, an Infineon Technologies Company](http://www.cypress.com)
* [Cypress Semiconductor GitHub](https://github.com/cypresssemiconductorco)
* [ModusToolbox](https://www.cypress.com/products/modustoolbox-software-environment)
* [PSoC 6 Code Examples using ModusToolbox IDE](https://github.com/cypresssemiconductorco/Code-Examples-for-ModusToolbox-Software)
* [PSoC 6 Middleware](https://github.com/cypresssemiconductorco/psoc6-middleware)
* [PSoC 6 Resources - KBA223067](https://community.cypress.com/docs/DOC-14644)

---
© Cypress Semiconductor Corporation, 2019-2020.
