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

#include "reach_nrf_connect.h"

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>

#include "reach-server.h"
#include "cr_stack.h"
#include "i3_log.h"
#include "app_version.h"
#include "reach_version.h"

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

// Default define values, which can be overridden in the .h file as needed
#ifndef BLE_TASK_STACK_SIZE
#define BLE_TASK_STACK_SIZE 8192
#endif // BLE_TASK_STACK_SIZE

#ifndef BLE_TASK_PRIORITY
#define BLE_TASK_PRIORITY 1
#endif // BLE_TASK_PRIORITY

#ifndef BLE_ADV_INTERVAL_MS
#define BLE_ADV_INTERVAL_MS 500
#elif (BLE_ADV_INTERVAL_MS < 2)
#error "BLE advertising interval must be at least 2ms"
#endif // BLE_ADV_INTERVAL_MS

// Need slightly more complicated logic for setting the advertising spacing
#ifndef BLE_ADV_INTERVAL_SPACING_MS
#if (BLE_ADV_INTERVAL_MS > 200)
#define BLE_ADV_INTERVAL_SPACING_MS 100
#elif (BLE_ADV_INTERVAL_MS > 20)
#define BLE_ADV_INTERVAL_SPACING_MS 10
#else
#define BLE_ADV_INTERVAL_SPACING_MS 1
#endif // (BLE_ADV_INTERVAL_MS > 10)
#else
#if (BLE_ADV_INTERVAL_MS < BLE_ADV_INTERVAL_SPACING_MS)
#error "BLE Advertising interval spacing cannot result in negative advertising intervals"
#endif // (BLE_ADV_INTERVAL_MS < BLE_ADV_INTERVAL_SPACING_MS)
#endif // BLE_ADV_INTERVAL_SPACING_MS

#ifndef BLE_TASK_ADVERTISING_PROCESSING_INTERVAL_MS
#define BLE_TASK_ADVERTISING_PROCESSING_INTERVAL_MS 100
#endif // BLE_TASK_ADVERTISING_PROCESSING_INTERVAL_MS

#ifndef BLE_TASK_CONNECTED_PROCESSING_INTERVAL_MS
#define BLE_TASK_CONNECTED_PROCESSING_INTERVAL_MS 5
#endif // BLE_TASK_CONNECTED_PROCESSING_INTERVAL_MS

#ifndef BLE_WRITE_CIRCULAR_BUFFER_SIZE
#define BLE_WRITE_CIRCULAR_BUFFER_SIZE 1
#endif // BLE_WRITE_CIRCULAR_BUFFER_SIZE

#ifndef REACH_SERVICE_UUID
#define REACH_SERVICE_UUID BT_UUID_128_ENCODE(0xedd59269, 0x79b3, 0x4ec2, 0xa6a2, 0x89bfb640f930)
#endif // REACH_UUID

#ifndef REACH_CHARACTERISTIC_UUID
#define REACH_CHARACTERISTIC_UUID BT_UUID_128_ENCODE(0xd42d1039, 0x1d11, 0x4f10, 0xbae6, 0x5f3b44cf6439)
#endif // REACH_CHARACTERISTIC_UUID

// Defines only needed internally
#define ADVERTISING_INTERVAL(ms) (((ms) * 8) / 5)
#define BLE_ADV_INTERVAL_MIN ADVERTISING_INTERVAL(BLE_ADV_INTERVAL_MS - BLE_ADV_INTERVAL_SPACING_MS)
#define BLE_ADV_INTERVAL_MAX ADVERTISING_INTERVAL(BLE_ADV_INTERVAL_MS + BLE_ADV_INTERVAL_SPACING_MS)

#define BT_LE_AD_LOW_POWER BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE, BLE_ADV_INTERVAL_MIN, BLE_ADV_INTERVAL_MAX, NULL)

#if (APP_ADVERTISED_NAME_LENGTH > 30)
#error "nRF Connect BLE advertised name length cannot be greater than 30 characters"
#endif

#define REACH_SERVICE_UUID_DECLARE BT_UUID_DECLARE_128(REACH_SERVICE_UUID)
#define REACH_CHARACTERISTIC_UUID_DECLARE BT_UUID_DECLARE_128(REACH_CHARACTERISTIC_UUID)

/*******************************************************************************
 ****************************   LOCAL  TYPES   *********************************
 ******************************************************************************/

#if (BLE_WRITE_CIRCULAR_BUFFER_SIZE > 1)
// Structures for basic circular buffers
typedef struct {
    uint8_t buf[CR_CODED_BUFFER_SIZE];
    size_t length;
} coded_buffer_t;

typedef struct {
    coded_buffer_t buffers[BLE_WRITE_CIRCULAR_BUFFER_SIZE];
    size_t head;
    size_t tail;
} circ_buf_t;
#endif

/*******************************************************************************
 *********************   LOCAL FUNCTION PROTOTYPES   ***************************
 ******************************************************************************/

// Main BLE task
static void ble_task(void *arg, void *param2, void *param3);

// Callbacks for BLE events
static void connected(struct bt_conn *conn, uint8_t err);
static void connect_work_handler(struct k_work *item);
static void disconnect_work_handler(struct k_work *item);
static void disconnected(struct bt_conn *conn, uint8_t reason);
static ssize_t read_reach(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, uint16_t len, uint16_t offset);
static ssize_t write_reach(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf, uint16_t len, uint16_t offset, uint8_t flags);
static void subscribe_reach(const struct bt_gatt_attr *attr, uint16_t value);

#if (BLE_WRITE_CIRCULAR_BUFFER_SIZE > 1)
// Functions for basic circular buffers
static void cb_add(circ_buf_t *cb, coded_buffer_t *c);
static bool cb_remove(circ_buf_t *cb, coded_buffer_t **c);
static uint32_t cb_get_size(circ_buf_t *cb);
#endif

// strnlen is technically a Linux function and is often not found by the compiler.
size_t strnlen( const char * s,size_t maxlen );

/*******************************************************************************
 ***************************  LOCAL VARIABLES   ********************************
 ******************************************************************************/

// Advertising data
char advertised_name[APP_ADVERTISED_NAME_LENGTH];
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, REACH_SERVICE_UUID),
};

// Scanning data
static struct bt_data sd[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, advertised_name, 0),
};

// Data for reading/writing the Reach characteristic
static uint8_t reach_data[244];
#if (BLE_WRITE_CIRCULAR_BUFFER_SIZE > 1)
static circ_buf_t ble_write_buffer;
#else
static volatile size_t reach_length = 1;
#endif

// Connection callback structure used by the stack
static struct bt_conn_cb connection_callbacks = {
    .connected = connected,
    .disconnected = disconnected,
};

// Service definition
BT_GATT_SERVICE_DEFINE(reach_service,
    BT_GATT_PRIMARY_SERVICE(REACH_SERVICE_UUID_DECLARE),
    BT_GATT_CHARACTERISTIC(REACH_CHARACTERISTIC_UUID_DECLARE,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP | BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
                           read_reach, write_reach, reach_data),
    BT_GATT_CCC(subscribe_reach, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)
);

// BLE task data
K_THREAD_STACK_DEFINE(ble_task_stack_area, BLE_TASK_STACK_SIZE);
static struct k_thread ble_task_data;
static k_tid_t ble_task_id;

// Data for connection and disconnection work
static struct k_work connect_work;
static struct k_work disconnect_work;

// State information
static bool ble_advertising_started = false;
static bool device_connected = false;
static bool device_subscribed = false;

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

void rnrfc_init(void)
{
    // Initialize the advertised name if it hasn't been set already
    if (advertised_name[0] == 0)
    {
        // Handling to make sure setting the advertised name is successful
        char temp[APP_ADVERTISED_NAME_LENGTH];
        strncpy(temp, CONFIG_BT_DEVICE_NAME, sizeof(temp));
        rnrfc_set_advertised_name(temp);
    }
    cr_test_sizes();

    cr_init();

    ble_task_id = k_thread_create(
        &ble_task_data, ble_task_stack_area,
        K_THREAD_STACK_SIZEOF(ble_task_stack_area),
        ble_task,
        NULL, NULL, NULL,
        BLE_TASK_PRIORITY,
        K_FP_REGS,
        K_NO_WAIT);

    int rval = bt_enable(NULL);
    if (rval)
    {
        I3_LOG(LOG_MASK_ERROR, "Bluetooth init failed (err %d)", rval);
        return;
    }

    k_work_init(&connect_work, connect_work_handler);
    k_work_init(&disconnect_work, disconnect_work_handler);

    bt_conn_cb_register(&connection_callbacks);

    rval = bt_le_adv_start(BT_LE_AD_LOW_POWER, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (rval)
    {
        I3_LOG(LOG_MASK_ERROR, "Bluetooth advertising failed to start (err %d)", rval);
        return;
    }
    ble_advertising_started = true;
}

int rnrfc_set_advertised_name(char *name)
{
    int rval = 0;
    size_t name_length = strnlen(name, sizeof(advertised_name) + 1);
    if (name_length > sizeof(advertised_name) || name_length == 0)
        return -1;
    memset(advertised_name, 0, sizeof(advertised_name));
    strncpy(advertised_name, name, sizeof(advertised_name));
    sd[0].data_len = (uint8_t) name_length;
    if (ble_advertising_started)
    {
        rval = bt_le_adv_stop();
        if (rval)
        {
            I3_LOG(LOG_MASK_ERROR, "Failed to stop BLE advertising, error %d", rval);
            rval = -2;
        }
        rval = bt_le_adv_start(BT_LE_AD_LOW_POWER, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
        if (rval)
        {
            I3_LOG(LOG_MASK_ERROR, "Failed to restart BLE advertising, error %d", rval);
            rval = -3;
        }
    }
    cr_set_advertised_name(advertised_name, name_length);
    return rval;
}

int crcb_send_coded_response(const uint8_t *respBuf, size_t respSize)
{
    if (respSize == 0)
    {
        I3_LOG(LOG_MASK_REACH, "%s: No bytes to send.  ", __FUNCTION__);
        return cr_ErrorCodes_NO_ERROR;
    }
    I3_LOG(LOG_MASK_REACH, TEXT_GREEN "%s: send %d bytes.", __FUNCTION__, respSize);
    if (device_subscribed)
    {   
        int rval = bt_gatt_notify(NULL, &reach_service.attrs[2], respBuf, (uint16_t) respSize);
        if (rval)
        {
            LOG_ERROR("Notify failed, error %d", rval);
            return cr_ErrorCodes_WRITE_FAILED;
        }
    }
    return 0;
}

#ifdef INCLUDE_FILE_SERVICE
int crcb_file_get_preferred_ack_rate(bool is_write)
{
    I3_LOG(LOG_MASK_WARN, "Logging can interfere with file write.");
    if (is_write)
#if (BLE_WRITE_CIRCULAR_BUFFER_SIZE > 1)
        return BLE_WRITE_CIRCULAR_BUFFER_SIZE - 1;
#else
        return 1;
#endif
    else
        return 0;
}
#endif // INCLUDE_FILE_SERVICE

void __attribute__((weak)) rnrfc_app_handle_ble_connection(void)
{
    // Do nothing
    return;
}

void __attribute__((weak)) rnrfc_app_handle_ble_disconnection(void)
{
    // Do nothing
    return;
}

/*******************************************************************************
 ***************************   LOCAL FUNCTIONS    ******************************
 ******************************************************************************/

static void ble_task(void *arg, void *param2, void *param3)
{
    I3_LOG(LOG_MASK_BLE, "Starting BLE task");
    while (1)
    {
        if (!device_connected)
        {
            k_msleep(BLE_TASK_ADVERTISING_PROCESSING_INTERVAL_MS);
        }
        else
        {
#if (BLE_WRITE_CIRCULAR_BUFFER_SIZE > 1)
            // Handle any incoming data from the circular buffer
            if (cb_get_size(&ble_write_buffer))
            {
                I3_LOG(LOG_MASK_BLE, "Process buffer");
                coded_buffer_t *temp;
                if (cb_remove(&ble_write_buffer, &temp))
                    cr_store_coded_prompt(temp->buf, temp->length);
                else
                    break;
            }
#endif
            // Handle any outgoing data
            cr_process(k_uptime_get_32());
            k_msleep(BLE_TASK_CONNECTED_PROCESSING_INTERVAL_MS);
        }
    }
}

static void connected(struct bt_conn *conn, uint8_t err)
{
    k_work_submit(&connect_work);
}

static void connect_work_handler(struct k_work *item)
{
    I3_LOG(LOG_MASK_BLE, "BLE connected");
    rnrfc_app_handle_ble_connection();
    cr_set_comm_link_connected(true);
    device_connected = true;

}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    k_work_submit(&disconnect_work);
}

#if (BLE_WRITE_CIRCULAR_BUFFER_SIZE > 1)
static void cb_add(circ_buf_t *cb, coded_buffer_t *c)
{
    size_t next_head = (cb->head + 1) % BLE_WRITE_CIRCULAR_BUFFER_SIZE;
    cb->buffers[cb->head] = *c;
    cb->head = next_head;
    if (cb->head == cb->tail)
    {
        cb->tail = (cb->tail + 1) % BLE_WRITE_CIRCULAR_BUFFER_SIZE;
    }
}

static bool cb_remove(circ_buf_t *cb, coded_buffer_t **c)
{
    if (cb->head != cb->tail) {
        *c = &cb->buffers[cb->tail];
        cb->tail = (cb->tail + 1) % BLE_WRITE_CIRCULAR_BUFFER_SIZE;
        return true;
    } else {
        return false;
    }
}

static uint32_t cb_get_size(circ_buf_t *cb)
{
    int temp = (cb->head - cb->tail);
    return temp < 0 ? temp + BLE_WRITE_CIRCULAR_BUFFER_SIZE:temp;
}
#endif

static void disconnect_work_handler(struct k_work *item)
{
    cr_set_comm_link_connected(false);
    device_connected = false;
    device_subscribed = false;
    rnrfc_app_handle_ble_disconnection();
    I3_LOG(LOG_MASK_BLE, "BLE disconnected");
}

static ssize_t read_reach(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf, uint16_t len, uint16_t offset)
{
    uint8_t *resp_buf;
    size_t resp_len;
    cr_get_coded_response_buffer(&resp_buf, &resp_len);
    I3_LOG(LOG_MASK_BLE, "Read request for reach. %d.", resp_len);
    memcpy(buf, resp_buf, resp_len);
    return resp_len;
}
static ssize_t write_reach(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
    // I3_LOG(LOG_MASK_BLE, "Write to reach.  Len %u", len);
#if (BLE_WRITE_CIRCULAR_BUFFER_SIZE > 1)
    // Store the data in the circular buffer, there must always be a process between stores
    coded_buffer_t temp;
    memcpy(temp.buf, buf, (size_t) len);
    temp.length = (size_t) len;
    cb_add(&ble_write_buffer, &temp);
#else
    cr_store_coded_prompt((uint8_t *) buf, (size_t) len);
    memcpy(reach_data, buf, len);
    reach_length = (size_t) len;
#endif
    return len;
}

static void subscribe_reach(const struct bt_gatt_attr *attr, uint16_t value)
{
    device_subscribed = (value == BT_GATT_CCC_NOTIFY);
    I3_LOG(LOG_MASK_BLE, "%s Reach characteristic", value == BT_GATT_CCC_NOTIFY ? "Subscribe to":"Unsubscribe from");
}