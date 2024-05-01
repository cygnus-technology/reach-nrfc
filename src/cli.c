/********************************************************************************************
 *
 * \date   2023
 *
 * \author i3 Product Development (JNP)
 *
 * \brief  Functions to handle the Reach CLI service
 *
 ********************************************************************************************/

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/fs/fs.h>
#include <ncs_version.h>

#include "reach-server.h"
#include "cr_stack.h"
#include "i3_log.h"
#include "app_version.h"
#include "main.h"

#define TEXT_CLI ""

#define CLI_TASK_STACK_SIZE 2048
#define CLI_TASK_PRIORITY 5

#define TOSTRING_LAYER_ONE(x) #x
#define TOSTRING(x) TOSTRING_LAYER_ONE(x)

static void cli_task(void *arg, void *param2, void *param3);

K_THREAD_STACK_DEFINE(cli_task_stack_area, CLI_TASK_STACK_SIZE);
struct k_thread cli_task_data;
k_tid_t cli_task_id;

#ifdef DEV_BUILD
static const char sAppVersion[] = TOSTRING(APP_MAJOR_VERSION) "." TOSTRING(APP_MINOR_VERSION) "." TOSTRING(APP_PATCH_VERSION) "-dev";
#else
static const char sAppVersion[] = TOSTRING(APP_MAJOR_VERSION) "." TOSTRING(APP_MINOR_VERSION) "." TOSTRING(APP_PATCH_VERSION);
#endif

static void slash(void);
static void lm(const char *input);

void cli_init(void)
{
    cli_task_id = k_thread_create(
        &cli_task_data, cli_task_stack_area,
        K_THREAD_STACK_SIZEOF(cli_task_stack_area),
        cli_task,
        NULL, NULL, NULL,
        CLI_TASK_PRIORITY,
        K_FP_REGS,
        K_NO_WAIT);
}

static void cli_task(void *arg, void *param2, void *param3)
{
    char input[64];
    uint8_t input_length = 0;
    const struct device *uart_dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);
    char clear_screen[] = "\033[2J\033]H";
    for (int i = 0; i < sizeof(clear_screen); i++)
        uart_poll_out(uart_dev, clear_screen[i]);
    print_versions();
    uart_poll_out(uart_dev, '>');
    while (1)
    {
        if (input_length == sizeof(input))
        {
            i3_log(LOG_MASK_WARN, "CLI input too long, clearing");
            memset(input, 0, sizeof(input));
            input_length = 0;
        }
        if (!uart_poll_in(uart_dev, &input[input_length]))
        {
            switch (input[input_length])
            {
                case '\r':
                    uart_poll_out(uart_dev, '\r');
                    uart_poll_out(uart_dev, '\n');
                    if (input_length == 0)
                    {
                        uart_poll_out(uart_dev, '>');
                        break; // No data, no need to call anything
                    }
                    input[input_length] = 0; // Null-terminate the string
                    crcb_cli_enter((const char *) input);
                    input_length = 0;
                    memset(input, 0, sizeof(input));
                    uart_poll_out(uart_dev, '>');
                    break;
                case '\n':
                    break; // Ignore, only expect '\r' for command execution
                case '\b':
                    // Received a backspace
                    if (input_length > 0)
                    {
                        input[--input_length] = 0;
                        uart_poll_out(uart_dev, '\b');
                        uart_poll_out(uart_dev, ' ');
                        uart_poll_out(uart_dev, '\b');
                    }
                    break;
                default:
                    // Still waiting for an input
                    uart_poll_out(uart_dev, input[input_length]);
                    if (input_length < sizeof(input))
                        input_length++;
                    break;
            }
        }
        k_msleep(10);
    }

}

const char *get_app_version()
{
    return sAppVersion;
}

void print_versions(void)
{
    i3_log(LOG_MASK_ALWAYS, TEXT_CLI "Reach nRF52840 Dongle demo, built %s, %s", __DATE__, __TIME__);
    i3_log(LOG_MASK_ALWAYS, TEXT_CLI "  nRF Connect SDK version %s", NCS_VERSION_STRING);
    i3_log(LOG_MASK_ALWAYS, TEXT_CLI "  Reach stack version %s", cr_get_reach_version());
    i3_log(LOG_MASK_ALWAYS, TEXT_CLI "  Reach protobuf version %s", cr_get_proto_version());
    i3_log(LOG_MASK_ALWAYS, TEXT_CLI "  App version %s", get_app_version());
}

int crcb_cli_enter(const char *ins)
{
    if (*ins == '\r' || *ins == '\n')
    {
        return 0;
    }

    if ((*ins == '?') || (!strncmp("help", ins, 4)) )
    {
        i3_log(LOG_MASK_ALWAYS, TEXT_GREEN "!!! Reach nRF52840 Dongle demo, built %s, %s", __DATE__, __TIME__);
        i3_log(LOG_MASK_ALWAYS, TEXT_GREEN "!!! App Version %s", get_app_version());
        i3_log(LOG_MASK_ALWAYS, TEXT_CLI "Commands:");
        i3_log(LOG_MASK_ALWAYS, TEXT_CLI "  ver : Print versions");
        i3_log(LOG_MASK_ALWAYS, TEXT_CLI "  /   : Display status");
        i3_log(LOG_MASK_ALWAYS, TEXT_CLI "  lm (<new log mask>): Print current log mask, or set a new log mask");
        return 0;
    }

    crcb_set_command_line(ins);
    // step through remote_command_table and execute if matching
    if (!strncmp("ver", ins, 3))
        print_versions();
    else if (!strncmp("/", ins, 1))
        slash();
    else if (!strncmp("lm", ins, 2))
        lm(ins);
    else
        i3_log(LOG_MASK_WARN, "CLI command '%s' not recognized.", ins, *ins);
    return 0;
}

static void slash(void)
{
    // File system statistics
    struct fs_statvfs fs_stats;
    int rval = fs_statvfs("/lfs", &fs_stats);
    if (rval)
        i3_log(LOG_MASK_ERROR, TEXT_CLI "Failed to get file system stats, error %d", rval);
    else
    {
        i3_log(LOG_MASK_ALWAYS, TEXT_CLI "File system: %u/%u blocks (%u bytes each) remaining", fs_stats.f_bfree, fs_stats.f_blocks, fs_stats.f_frsize);
    }

    // System information
    bt_addr_le_t ble_id;
    size_t id_count = 1;
    bt_id_get(&ble_id, &id_count);
    i3_log(LOG_MASK_ALWAYS, TEXT_CLI "BLE Device Address: %02X:%02X:%02X:%02X:%02X:%02X",
        ble_id.a.val[5], ble_id.a.val[4], ble_id.a.val[3], ble_id.a.val[2], ble_id.a.val[1], ble_id.a.val[0]);
    i3_log(LOG_MASK_ALWAYS, TEXT_CLI "Uptime: %.3f seconds", ((float) k_uptime_get()) / 1000);

    // Reach information
    i3_log(LOG_MASK_ALWAYS, TEXT_CLI "Current log mask: 0x%x", i3_log_get_mask());
  #ifdef ENABLE_REMOTE_CLI
    if (i3_log_get_remote_cli_enable())
        i3_log(LOG_MASK_ALWAYS, TEXT_CLI "Remote CLI support enabled.");
    else
        i3_log(LOG_MASK_ALWAYS, TEXT_CLI "Remote CLI support built but not enabled.");
  #else
    i3_log(LOG_MASK_ALWAYS, TEXT_CLI "!!! Remote CLI NOT support built in.");
  #endif
}

static void lm(const char *input)
{
    uint32_t lm = 0;
    int args = sscanf(input, "lm %x", &lm);
    if (args != 1)
    {
        lm = i3_log_get_mask();
        // Print current log mask
        i3_log(LOG_MASK_ALWAYS, TEXT_CLI "Current log mask: 0x%x", lm);
#ifndef NO_REACH_LOGGING
        // print information about log mask
        i3_log(LOG_MASK_ALWAYS, TEXT_CLI "  ALWAYS, ERROR and WARN are enabled");
        if (lm & LOG_MASK_WEAK) i3_log(LOG_MASK_ALWAYS, TEXT_CLI "  mask 0x%x: WEAK is enabled", LOG_MASK_WEAK);
        else i3_log(LOG_MASK_ALWAYS, TEXT_CLI "    mask 0x%x: WEAK is disabled", LOG_MASK_WEAK);
        if (lm & LOG_MASK_WIRE) i3_log(LOG_MASK_ALWAYS, TEXT_CLI "  mask 0x%x: WIRE is enabled", LOG_MASK_WIRE);
        else i3_log(LOG_MASK_ALWAYS, TEXT_CLI "    mask 0x%x: WIRE is disabled", LOG_MASK_WIRE);
        if (lm & LOG_MASK_REACH) i3_log(LOG_MASK_ALWAYS, TEXT_CLI "  mask 0x%x: REACH is enabled", LOG_MASK_REACH);
        else i3_log(LOG_MASK_ALWAYS, TEXT_CLI "    mask 0x%x: REACH is disabled", LOG_MASK_REACH);
        if (lm & LOG_MASK_PARAMS) i3_log(LOG_MASK_ALWAYS, TEXT_CLI "  mask 0x%x: PARAMS is enabled", LOG_MASK_PARAMS);
        else i3_log(LOG_MASK_ALWAYS, TEXT_CLI "    mask 0x%x: PARAMS is disabled", LOG_MASK_PARAMS);
        if (lm & LOG_MASK_FILES) i3_log(LOG_MASK_ALWAYS, TEXT_CLI "  mask 0x%x: FILES is enabled", LOG_MASK_FILES);
        else i3_log(LOG_MASK_ALWAYS, TEXT_CLI "    mask 0x%x: FILES is disabled", LOG_MASK_FILES);
        if (lm & LOG_MASK_BLE) i3_log(LOG_MASK_ALWAYS, TEXT_CLI "  mask 0x%x: BLE is enabled", LOG_MASK_BLE);
        else i3_log(LOG_MASK_ALWAYS, TEXT_CLI "    mask 0x%x: BLE is disabled", LOG_MASK_BLE);
        i3_log(LOG_MASK_ALWAYS, TEXT_CLI "  Other Valid log masks:");
        i3_log(LOG_MASK_ALWAYS, TEXT_CLI "    LOG_MASK_ACME               0x4000");
        i3_log(LOG_MASK_ALWAYS, TEXT_CLI "    LOG_MASK_DEBUG              0x8000");
        i3_log(LOG_MASK_ALWAYS, TEXT_CLI "    LOG_MASK_TIMEOUT           0x10000");
        i3_log(LOG_MASK_ALWAYS, TEXT_CLI "    LOG_MASK_DATASTREAM_DEBUG 0x100000");
        i3_log(LOG_MASK_ALWAYS, TEXT_CLI "    LOG_MASK_ZIGBEE_DEBUG     0x200000");
        i3_log(LOG_MASK_ALWAYS, TEXT_CLI "    LOG_MASK_ZIGBEE_OTA_DEBUG 0x400000");
#else
        // Logging is typically disabled to save space, so don't waste it with a bunch of printouts
        i3_log(LOG_MASK_WARN, TEXT_CLI "Log mask is of limited use, as logging is disabled");
#endif
    }
    else
    {
        i3_log_set_mask(lm);
        i3_log(LOG_MASK_ALWAYS, TEXT_CLI "The log mask is now 0x%x", lm);
#ifdef NO_REACH_LOGGING
        i3_log(LOG_MASK_WARN, TEXT_CLI "Log mask is of limited use, as logging is disabled");
#endif
    }
}