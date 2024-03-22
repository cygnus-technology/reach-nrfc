/*
 * Copyright (c) 2023-2024 i3 Product Development
 * 
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file      reach_nrf_connect.h
 * @brief     Integration of nRF Connect BLE features for Cygnus Reach
 * 
 * @copyright (c) Copyright 2024 i3 Product Development. All Rights Reserved.
 */


#ifndef _REACH_H_
#define _REACH_H_

#include <stddef.h>
#include <zephyr/bluetooth/uuid.h>

#include "reach-server.h"

// TODO: Make this work properly with our system, this is based on https://stackoverflow.com/a/35152306
#ifdef _DOXYGEN_
    /** @brief The stack size (in bytes) allocated to the BLE task which processes Reach communications */
    #define BLE_TASK_STACK_SIZE 8192

    /** @brief The priority for the BLE task.  By default, this is the highest cooperative priority value */
    #define BLE_TASK_PRIORITY 1

    /** @brief How often the device will advertise.  A shorter interval will consume more power */
    #define BLE_ADV_INTERVAL_MS 500

    /** @brief BLE advertising includes a minimum and maximum interval to help with scheduling.
     * The spacing must be less than the interval, as the minimum interval must be > 0.
     * @note Minimum advertising interval = BLE_ADV_INTERVAL_MS - BLE_ADV_INTERVAL_SPACING_MS
     * @note Maximum advertising interval = BLE_ADV_INTERVAL_MS + BLE_ADV_INTERVAL_SPACING_MS
     */
    #define BLE_ADV_INTERVAL_SPACING_MS 100

    /** @brief How often the device will check for new data when there is no BLE connection. */
    #define BLE_TASK_ADVERTISING_PROCESSING_INTERVAL_MS 100

    /** @brief How often the device will check for new data when BLE is connected. */
    #define BLE_TASK_CONNECTED_PROCESSING_INTERVAL_MS 5

    /** @brief The size of the circular buffer used to handle incoming writes.
     * @note This can generally be set to 1 unless file writes are being done,
     * in which case values > 2 will increase the write speed at the cost of RAM usage
     */
    #define BLE_WRITE_CIRCULAR_BUFFER_SIZE 1

    /** @brief The UUID for the advertised Reach service.  Set to the standard Reach UUID by default.
     * @note Changing this from the default will prevent this device from working with the generic Reach app
     */
    #define REACH_SERVICE_UUID BT_UUID_128_ENCODE(0xedd59269, 0x79b3, 0x4ec2, 0xa6a2, 0x89bfb640f930)

    /** @brief The UUID for the Reach characteristic.  Set to the standard Reach characteristic UUID by default.
     * @note Changing this from the default will prevent this device from working with the generic Reach app
     */
    #define REACH_CHARACTERISTIC_UUID BT_UUID_128_ENCODE(0xd42d1039, 0x1d11, 0x4f10, 0xbae6, 0x5f3b44cf6439)
#endif

// To change any of the defines described above, define them here
#define BLE_WRITE_CIRCULAR_BUFFER_SIZE 10

/**
* @brief Initializes the nRF Connect Reach BLE implementation
*/
void rnrfc_init();

/**
* @brief Sets the advertised name
* @param name The new advertised name.  This cannot be longer than APP_ADVERTISED_NAME_LENGTH
* @return 0 on success, -1 if the name is invalid, or -2 if the stack-level name change failed
*/
int rnrfc_set_advertised_name(char *name);

/**
* @brief A callback for when a device connects via BLE, which can be used for app-specific actions
*/
void rnrfc_app_handle_ble_connection(void);

/**
* @brief A callback for when a device disconnects via BLE, which can be used for app-specific actions
*/
void rnrfc_app_handle_ble_disconnection(void);

#endif // _REACH_H_
