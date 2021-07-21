/***********************************************************************************************//**
 * \file cy_rgb_led.c
 *
 * Description:
 * Provides APIs for controlling the RGB LED on the Cypress kits.
 *
 ***************************************************************************************************
 * \copyright
 * Copyright 2018-2021 Cypress Semiconductor Corporation
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **************************************************************************************************/

#include "cy_rgb_led.h"
#include "cy_syspm.h"

#if defined(__cplusplus)
extern "C" {
#endif

// PWM period in micro seconds
#define CY_RGB_LED_PWM_PERIOD_US          (255u)

// RGB LED OFF
#define CY_RGB_LED_OFF                    (0u)

// RGB LED ON
#define CY_RGB_LED_ON                     (1u)

// PWM clock Hz
#define CY_RGB_LED_TCPWM_CLK_HZ      (1000000u)

// Modes to be skipped for the RGB LED during low power mode transition
#define CY_RGB_LED_TCPWM_LP_MODE_CHECK_SKIP \
    (CYHAL_SYSPM_CHECK_READY | CYHAL_SYSPM_CHECK_FAIL | CYHAL_SYSPM_BEFORE_TRANSITION | CYHAL_SYSPM_AFTER_TRANSITION)

// This is the handler function to ensure proper operation of RGB LED during  device power mode
// transition.
bool cy_rgb_led_lp_readiness(cyhal_syspm_callback_state_t,
                             cyhal_syspm_callback_mode_t, void* callback_arg);

// TCPWM instances for RGB LED control
static cyhal_pwm_t pwm_red_obj;
static cyhal_pwm_t pwm_green_obj;
static cyhal_pwm_t pwm_blue_obj;

// Clock instance
static cyhal_clock_t clk_pwm;

// Variables used to track the LED state (ON/OFF, Color, Brightness)
static uint32_t led_color = CY_RGB_LED_COLOR_OFF;
static uint8_t  led_brightness;
static bool     rgb_led_state = CY_RGB_LED_OFF;

// Variable to track the active logic of the RGB LED
static bool rgb_led_active_logic = CY_RGB_LED_ACTIVE_LOW;

// Structure with syspm configuration element (refer to cy_syspm.h)
cyhal_syspm_callback_data_t lp_config =
{
    .callback       = cy_rgb_led_lp_readiness,
    .states         = CYHAL_SYSPM_CB_CPU_DEEPSLEEP,
    .ignore_modes   =
        (cyhal_syspm_callback_mode_t)(CYHAL_SYSPM_CHECK_FAIL |
                                      CYHAL_SYSPM_BEFORE_TRANSITION |
                                      CYHAL_SYSPM_AFTER_TRANSITION),

    .args           = NULL,
    .next           = NULL
};

//--------------------------------------------------------------------------------------------------
// cy_rgb_led_register_lp_cb
//
// This function registers the handler to take necessary actions (for the proper
// operation of RGB LED functionality) during device power mode transition
// (Deep Sleep to Active and vice-versa).
//--------------------------------------------------------------------------------------------------
static cy_rslt_t cy_rgb_led_register_lp_cb(void)
{
    cyhal_syspm_register_callback(&lp_config);
    return CY_RSLT_SUCCESS;
}


//--------------------------------------------------------------------------------------------------
// cy_rgb_led_lp_readiness
//
// This is the handler function to ensure proper operation of RGB LED during
// device power mode transition (Deep Sleep to Active and vice-versa). This
// low power callback function is registered as part of \ref cy_rgb_led_init
// function.
//--------------------------------------------------------------------------------------------------
bool cy_rgb_led_lp_readiness(cyhal_syspm_callback_state_t state,
                             cyhal_syspm_callback_mode_t mode, void* callback_arg)
{
    bool returnValue = false;

    // Remove unused parameter warning
    CY_UNUSED_PARAMETER(state);
    CY_UNUSED_PARAMETER(callback_arg);

    if ((mode == CYHAL_SYSPM_CHECK_READY) && (rgb_led_state == CY_RGB_LED_OFF))
    {
        returnValue = true;
    }
    return returnValue;
}


//--------------------------------------------------------------------------------------------------
// cy_rgb_led_init
//
// Initializes three TCPWMs in PWM mode for RGB LED control. If any of the TCPWMs
// is not available (i.e. reserved by the hardware resource manager), then
// RGB LED initialization fails.
//--------------------------------------------------------------------------------------------------
cy_rslt_t cy_rgb_led_init(cyhal_gpio_t pin_red, cyhal_gpio_t pin_green, cyhal_gpio_t pin_blue,
                          bool led_active_logic)
{
    cy_rslt_t             result = CY_RSLT_SUCCESS;
    bool green_success = false, blue_success = false, red_success = false, clock_success = false;

    rgb_led_active_logic = led_active_logic;
    // Allocate and assign the clock for TCPWMs for RGB LED control
    result = cyhal_clock_allocate(&clk_pwm, CYHAL_CLOCK_BLOCK_PERIPHERAL_16BIT);
    if (result == CY_RSLT_SUCCESS)
    {
        // if the clock was allocated set the bool to true
        clock_success = true;
        result = cyhal_clock_set_frequency(&clk_pwm, CY_RGB_LED_TCPWM_CLK_HZ, NULL);
    }
    if (result == CY_RSLT_SUCCESS)
    {
        result = cyhal_clock_set_enabled(&clk_pwm, true, true);
    }
    if (result == CY_RSLT_SUCCESS)
    {
        // Attempt to initialize PWM to control Red LED.
        result = cyhal_pwm_init(&pwm_red_obj, pin_red, &clk_pwm);
    }
    if (result == CY_RSLT_SUCCESS)
    {
        red_success = true;
        // Attempt to initialize PWM to control Green LED.
        result = cyhal_pwm_init(&pwm_green_obj, pin_green, &clk_pwm);
    }
    if (result == CY_RSLT_SUCCESS)
    {
        green_success = true;
        // Attempt to initialize PWM to control Blue LED.
        result = cyhal_pwm_init(&pwm_blue_obj, pin_blue, &clk_pwm);
    }
    if (result == CY_RSLT_SUCCESS)
    {
        blue_success = true;
        result = cy_rgb_led_register_lp_cb();
    }
    if (result != CY_RSLT_SUCCESS)
    {
        //free each pwm that was successfully initialized
        if (red_success)
        {
            cyhal_pwm_free(&pwm_red_obj);
        }
        if (green_success)
        {
            cyhal_pwm_free(&pwm_green_obj);
        }
        if (blue_success)
        {
            cyhal_pwm_free(&pwm_blue_obj);
        }
        if (clock_success)
        {
            // if the clock was allocated disable and free the clock
            cyhal_clock_set_enabled(&clk_pwm, false, true);
            cyhal_clock_free(&clk_pwm);
        }
    }
    return result;
}


//--------------------------------------------------------------------------------------------------
// cy_rgb_led_deinit
//
// Deinitialize the TCPWM instances used for RGB LED control.
//--------------------------------------------------------------------------------------------------
void cy_rgb_led_deinit(void)
{
    // Deinitialize and free all the TCPWM instances used for RGB LED control
    cyhal_pwm_free(&pwm_red_obj);
    cyhal_pwm_free(&pwm_green_obj);
    cyhal_pwm_free(&pwm_blue_obj);

    cyhal_clock_set_enabled(&clk_pwm, false, true);
    //Disable and free the allocated clock
    cyhal_clock_free(&clk_pwm);

    // De-register the low power handler
    cyhal_syspm_unregister_callback(&lp_config);
}


//--------------------------------------------------------------------------------------------------
// cy_rgb_led_on
//
// This function turns ON the RGB LED with specified color and brightness.
//--------------------------------------------------------------------------------------------------
void cy_rgb_led_on(uint32_t color, uint8_t brightness)
{
    rgb_led_state = CY_RGB_LED_ON;
    led_color     = color;

    // Turn on the PWMs
    cyhal_pwm_start(&pwm_red_obj);
    cyhal_pwm_start(&pwm_green_obj);
    cyhal_pwm_start(&pwm_blue_obj);
    // Set the RGB LED brightness
    cy_rgb_led_set_brightness(brightness);
}


//--------------------------------------------------------------------------------------------------
// cy_rgb_led_off
//
// This function turns OFF the RGB LED.
//--------------------------------------------------------------------------------------------------
void cy_rgb_led_off(void)
{
    rgb_led_state = CY_RGB_LED_OFF;

    // Turn off the PWMs
    cyhal_pwm_stop(&pwm_red_obj);
    cyhal_pwm_stop(&pwm_green_obj);
    cyhal_pwm_stop(&pwm_blue_obj);
}


//--------------------------------------------------------------------------------------------------
// cy_rgb_led_set_color
//
// This function sets the RGB LED color.
// The brightness of each LED is varied by changing the ON duty cycle of the PWM
// output. Using different combination of brightness for each of the RGB component,
// different colors can be generated.
//--------------------------------------------------------------------------------------------------
void cy_rgb_led_set_color(uint32_t color)
{
    led_color = color;

    uint32_t pwm_red_pulse_width =
        (led_brightness * (uint8_t)(led_color >> CY_RGB_LED_RED_POS))/CY_RGB_LED_MAX_BRIGHTNESS;
    uint32_t pwm_green_pulse_width =
        (led_brightness * (uint8_t)(led_color >> CY_RGB_LED_GREEN_POS))/CY_RGB_LED_MAX_BRIGHTNESS;
    uint32_t pwm_blue_pulse_width =
        (led_brightness * (uint8_t)(led_color >> CY_RGB_LED_BLUE_POS))/CY_RGB_LED_MAX_BRIGHTNESS;

    if (rgb_led_active_logic == CY_RGB_LED_ACTIVE_LOW)
    {
        pwm_red_pulse_width   = CY_RGB_LED_PWM_PERIOD_US - pwm_red_pulse_width;
        pwm_green_pulse_width = CY_RGB_LED_PWM_PERIOD_US - pwm_green_pulse_width;
        pwm_blue_pulse_width  = CY_RGB_LED_PWM_PERIOD_US - pwm_blue_pulse_width;
    }

    cyhal_pwm_set_period(&pwm_red_obj, CY_RGB_LED_PWM_PERIOD_US, pwm_red_pulse_width);
    cyhal_pwm_set_period(&pwm_green_obj, CY_RGB_LED_PWM_PERIOD_US, pwm_green_pulse_width);
    cyhal_pwm_set_period(&pwm_blue_obj, CY_RGB_LED_PWM_PERIOD_US, pwm_blue_pulse_width);
}


//--------------------------------------------------------------------------------------------------
// cy_rgb_led_set_brightness
//
// This function sets the RGB LED brightness.
//--------------------------------------------------------------------------------------------------
void cy_rgb_led_set_brightness(uint8_t brightness)
{
    led_brightness =
        (brightness < CY_RGB_LED_MAX_BRIGHTNESS) ? brightness : CY_RGB_LED_MAX_BRIGHTNESS;
    cy_rgb_led_set_color(led_color);
}


//--------------------------------------------------------------------------------------------------
// cy_rgb_led_get_color
//
// Provides the current color of the RGB LED.
//--------------------------------------------------------------------------------------------------
uint32_t cy_rgb_led_get_color(void)
{
    if (rgb_led_state == CY_RGB_LED_OFF)
    {
        return CY_RGB_LED_COLOR_OFF;
    }
    else
    {
        return led_color;
    }
}


//--------------------------------------------------------------------------------------------------
// cy_rgb_led_get_brightness
//
// Provides the current brightness of the RGB LED.
//--------------------------------------------------------------------------------------------------
uint8_t cy_rgb_led_get_brightness(void)
{
    if (rgb_led_state == CY_RGB_LED_OFF)
    {
        return 0;
    }
    else
    {
        return led_brightness;
    }
}


//--------------------------------------------------------------------------------------------------
// cy_rgb_led_toggle
//
// Toggles the RGB LED on the board
//--------------------------------------------------------------------------------------------------
void cy_rgb_led_toggle(void)
{
    if (rgb_led_state == CY_RGB_LED_OFF)
    {
        cy_rgb_led_on(led_color, led_brightness);
    }
    else
    {
        cy_rgb_led_off();
    }
}


#if defined(__cplusplus)
}
#endif
