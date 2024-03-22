/********************************************************************************************
 *
 * \date   2024
 *
 * \author i3 Product Development (JNP)
 *
 * \brief  Setup and main UI code for the nRF Connect demo.  Based on the BLE throughput sample code.
 *
 ********************************************************************************************/

#ifndef MAIN_H_
#define MAIN_H_

#include <stdbool.h>
#include <stdint.h>

void main_enable_identify(bool en);
bool main_identify_enabled(void);
void main_set_identify_interval(float seconds);
bool main_get_identify_led_on(void);
uint8_t main_get_rgb_led_state(void);
void main_set_rgb_led_state(uint8_t state);
bool main_get_button_pressed(void);

#endif /* MAIN_H_ */
