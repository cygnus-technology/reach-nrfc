/********************************************************************************************
 *    _ ____  ___             _         _     ___              _                        _
 *   (_)__ / | _ \_ _ ___  __| |_  _ __| |_  |   \ _____ _____| |___ _ __ _ __  ___ _ _| |_
 *   | ||_ \ |  _/ '_/ _ \/ _` | || / _|  _| | |) / -_) V / -_) / _ \ '_ \ '  \/ -_) ' \  _|
 *   |_|___/ |_| |_| \___/\__,_|\_,_\__|\__| |___/\___|\_/\___|_\___/ .__/_|_|_\___|_||_\__|
 *                                                                  |_|
 *                           -----------------------------------
 *                          Copyright i3 Product Development 2024
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
 *
 * \brief A minimal command-line interface implementation
 *
 * Original Author: Chuck Peplinski
 * Script Authors: Joseph Peplinski and Andrew Carlson
 *
 * Generated with version 1.0.0 of the C code generator
 *
 ********************************************************************************************/

/********************************************************************************************
 *************************************     Includes     *************************************
 *******************************************************************************************/

#include "cli.h"
#include "cr_stack.h"
#include "i3_log.h"

/* User code start [cli.c: User Includes] */
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/fs/fs.h>
#include <ncs_version.h>

#include "app_version.h"
#include "main.h"
/* User code end [cli.c: User Includes] */

/********************************************************************************************
 *************************************     Defines     **************************************
 *******************************************************************************************/

#ifndef CLI_MAX_LINE_LENGTH
#define CLI_MAX_LINE_LENGTH 64
#endif // CLI_MAX_LINE_LENGTH

/* User code start [cli.c: User Defines] */
#define CLI_TASK_STACK_SIZE 2048
#define CLI_TASK_PRIORITY 5
/* User code end [cli.c: User Defines] */

/********************************************************************************************
 ************************************     Data Types     ************************************
 *******************************************************************************************/

/* User code start [cli.c: User Data Types] */
/* User code end [cli.c: User Data Types] */

/********************************************************************************************
 *********************************     Global Variables     *********************************
 *******************************************************************************************/

/* User code start [cli.c: User Global Variables] */
/* User code end [cli.c: User Global Variables] */

/********************************************************************************************
 ***************************     Local Function Declarations     ****************************
 *******************************************************************************************/

static void cli_write_prompt(void);
static void cli_write(char *text);
static void cli_write_char(char c);
static bool cli_read_char(char *received);

/* User code start [cli.c: User Local Function Declarations] */
static void cli_task(void *arg, void *param2, void *param3);
static void print_versions(void);
static void slash(void);
static void lm(const char *input);
/* User code end [cli.c: User Local Function Declarations] */

/********************************************************************************************
 ******************************     Local/Extern Variables     ******************************
 *******************************************************************************************/

static char sInput[CLI_MAX_LINE_LENGTH];
static uint8_t sInputLength = 0;

/* User code start [cli.c: User Local/Extern Variables] */
static K_THREAD_STACK_DEFINE(sCliTaskStackArea, CLI_TASK_STACK_SIZE);
static struct k_thread sCliTaskData;
k_tid_t sCliTaskId;
static const struct device *sUartDevice = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);
/* User code end [cli.c: User Local/Extern Variables] */

/********************************************************************************************
 *********************************     Global Functions     *********************************
 *******************************************************************************************/

void cli_init(void)
{
    /* User code start [CLI: Init] */
    sCliTaskId = k_thread_create(
        &sCliTaskData, sCliTaskStackArea,
        K_THREAD_STACK_SIZEOF(sCliTaskStackArea),
        cli_task,
        NULL, NULL, NULL,
        CLI_TASK_PRIORITY,
        K_FP_REGS,
        K_NO_WAIT);
    /* User code end [CLI: Init] */
}

/**
 * Gets and processes data from the command line
 * @return True if CLI data was received during the poll (from sources other than BLE), or false otherwise.
 */
bool cli_poll(void)
{
    if (sInputLength == sizeof(sInput))
    {
        i3_log(LOG_MASK_WARN, "CLI input too long, clearing");
        memset(sInput, 0, sizeof(sInput));
        sInputLength = 0;
        cli_write_prompt();
    }
    if (cli_read_char(&sInput[sInputLength]))
    {
        switch (sInput[sInputLength])
        {
            case '\r':
                cli_write("\r\n");
                if (sInputLength == 0)
                {
                    cli_write_prompt();
                    break; // No data, no need to call anything
                }
                sInput[sInputLength] = 0; // Null-terminate the string
                crcb_cli_enter((const char*) sInput);
                sInputLength = 0;
                memset(sInput, 0, sizeof(sInput));
                cli_write_prompt();
                break;
            case '\n':
                break; // Ignore, only expect '\r' for command execution
            case '\b':
                // Received a backspace
                if (sInputLength > 0)
                {
                    sInput[--sInputLength] = 0;
                    cli_write("\b \b");
                }
                break;
            default:
                // Still waiting for an input
                cli_write_char(sInput[sInputLength]);
                if (sInputLength < sizeof(sInput))
                    sInputLength++;
                break;
        }
    return true;
    }
    return false;
}

/* User code start [cli.c: User Global Functions] */
/* User code end [cli.c: User Global Functions] */

/********************************************************************************************
 *************************     Cygnus Reach Callback Functions     **************************
 *******************************************************************************************/

int crcb_cli_enter(const char *ins)
{
    if (*ins == '\r' || *ins == '\n')
    {
        return 0;
    }

    if ((*ins == '?') || (!strncmp("help", ins, 4)))
        {
        i3_log(LOG_MASK_ALWAYS, "  ver: Print versions");
        i3_log(LOG_MASK_ALWAYS, "  /: Display status");
        i3_log(LOG_MASK_ALWAYS, "  lm (<new log mask>): Print current log mask, or set a new log mask");
        /* User code start [CLI: Custom help handling] */
        /* User code end [CLI: Custom help handling] */
        return 0;
    }

    crcb_set_command_line(ins);
    // step through remote_command_table and execute if matching
    if (!strncmp("ver", ins, 3))
    {
        /* User code start [CLI: 'ver' handler] */
        print_versions();
        /* User code end [CLI: 'ver' handler] */
    }
    else if (!strncmp("/", ins, 1))
    {
        /* User code start [CLI: '/' handler] */
        slash();
        /* User code end [CLI: '/' handler] */
    }
    else if (!strncmp("lm", ins, 2))
    {
        /* User code start [CLI: 'lm' handler] */
        lm(ins);
        /* User code end [CLI: 'lm' handler] */
    }
    /* User code start [CLI: Custom command handling] */
    /* User code end [CLI: Custom command handling] */
    else
        i3_log(LOG_MASK_WARN, "CLI command '%s' not recognized.", ins, *ins);
    return 0;
}

/* User code start [cli.c: User Cygnus Reach Callback Functions] */
/* User code end [cli.c: User Cygnus Reach Callback Functions] */

/********************************************************************************************
 *********************************     Local Functions     **********************************
 *******************************************************************************************/

static void cli_write_prompt(void)
{
    /* User code start [CLI: Write Prompt]
     * This is called after a command is sent and processed, indicating that the CLI is ready for a new prompt.
     * A typical implementation of this is to send a single '>' character. */
    cli_write_char('>');
    /* User code end [CLI: Write Prompt] */
}

static void cli_write(char *text)
{
    /* User code start [CLI: Write]
     * This is where other output sources should be handled (for example, writing to a UART port)
     * This is called for outputs which are not necessary via BLE, such as clearing lines or handling backspaces */
    int i = 0;
    while (text[i] != 0)
        uart_poll_out(sUartDevice, text[i++]);
    /* User code end [CLI: Write] */
}

static void cli_write_char(char c)
{
    /* User code start [CLI: Write Char]
     * This is used to write single characters, which may be handled differently from longer strings. */
    uart_poll_out(sUartDevice, c);
    /* User code end [CLI: Write Char] */
}

static bool cli_read_char(char *received)
{
    /* User code start [CLI: Read]
     * This is where other input sources (such as a UART) should be handled.
     * This should be non-blocking, and return true if a character was received, or false if not. */
    return (uart_poll_in(sUartDevice, (unsigned char *) received) == 0);
    /* User code end [CLI: Read] */
}

/* User code start [cli.c: User Local Functions] */

static void cli_task(void *arg, void *param2, void *param3)
{
    cli_write("\033[2J\033[H");
    print_versions();
    cli_write_prompt();
    while (1)
    {
        cli_poll();
        k_msleep(10);
    }

}

static void print_versions(void)
{
    i3_log(LOG_MASK_ALWAYS, "Reach nRF52840 Dongle demo, built %s, %s", __DATE__, __TIME__);
    i3_log(LOG_MASK_ALWAYS, "  nRF Connect SDK version %s", NCS_VERSION_STRING);
    i3_log(LOG_MASK_ALWAYS, "  Reach stack version %s", cr_get_reach_version());
    i3_log(LOG_MASK_ALWAYS, "  Reach protobuf version %s", cr_get_proto_version());
    i3_log(LOG_MASK_ALWAYS, "  App version %s", get_app_version());
}

static void slash(void)
{
    // File system statistics
    struct fs_statvfs fs_stats;
    int rval = fs_statvfs("/lfs", &fs_stats);
    if (rval)
        i3_log(LOG_MASK_ERROR, "Failed to get file system stats, error %d", rval);
    else
    {
        i3_log(LOG_MASK_ALWAYS, "File system: %u/%u blocks (%u bytes each) remaining", fs_stats.f_bfree, fs_stats.f_blocks, fs_stats.f_frsize);
    }

    // System information
    bt_addr_le_t ble_id;
    size_t id_count = 1;
    bt_id_get(&ble_id, &id_count);
    i3_log(LOG_MASK_ALWAYS, "BLE Device Address: %02X:%02X:%02X:%02X:%02X:%02X",
        ble_id.a.val[5], ble_id.a.val[4], ble_id.a.val[3], ble_id.a.val[2], ble_id.a.val[1], ble_id.a.val[0]);
    i3_log(LOG_MASK_ALWAYS, "Uptime: %.3f seconds", ((float) k_uptime_get()) / 1000);

    // Reach information
    i3_log(LOG_MASK_ALWAYS, "Current log mask: 0x%x", i3_log_get_mask());
}

static void lm(const char *input)
{
    uint32_t lm = 0;
    int args = sscanf(input, "lm %x", &lm);
    if (args != 1)
    {
        lm = i3_log_get_mask();
        // Print current log mask
        i3_log(LOG_MASK_ALWAYS, "Current log mask: 0x%x", lm);
#ifndef NO_REACH_LOGGING
        // print information about log mask
        i3_log(LOG_MASK_ALWAYS, "  ALWAYS, ERROR and WARN are enabled");
        if (lm & LOG_MASK_WEAK) i3_log(LOG_MASK_ALWAYS, "  mask 0x%x: WEAK is enabled", LOG_MASK_WEAK);
        else i3_log(LOG_MASK_ALWAYS, "    mask 0x%x: WEAK is disabled", LOG_MASK_WEAK);
        if (lm & LOG_MASK_WIRE) i3_log(LOG_MASK_ALWAYS, "  mask 0x%x: WIRE is enabled", LOG_MASK_WIRE);
        else i3_log(LOG_MASK_ALWAYS, "    mask 0x%x: WIRE is disabled", LOG_MASK_WIRE);
        if (lm & LOG_MASK_REACH) i3_log(LOG_MASK_ALWAYS, "  mask 0x%x: REACH is enabled", LOG_MASK_REACH);
        else i3_log(LOG_MASK_ALWAYS, "    mask 0x%x: REACH is disabled", LOG_MASK_REACH);
        if (lm & LOG_MASK_PARAMS) i3_log(LOG_MASK_ALWAYS, "  mask 0x%x: PARAMS is enabled", LOG_MASK_PARAMS);
        else i3_log(LOG_MASK_ALWAYS, "    mask 0x%x: PARAMS is disabled", LOG_MASK_PARAMS);
        if (lm & LOG_MASK_FILES) i3_log(LOG_MASK_ALWAYS, "  mask 0x%x: FILES is enabled", LOG_MASK_FILES);
        else i3_log(LOG_MASK_ALWAYS, "    mask 0x%x: FILES is disabled", LOG_MASK_FILES);
        if (lm & LOG_MASK_BLE) i3_log(LOG_MASK_ALWAYS, "  mask 0x%x: BLE is enabled", LOG_MASK_BLE);
        else i3_log(LOG_MASK_ALWAYS, "    mask 0x%x: BLE is disabled", LOG_MASK_BLE);
        i3_log(LOG_MASK_ALWAYS, "  Other Valid log masks:");
        i3_log(LOG_MASK_ALWAYS, "    LOG_MASK_ACME               0x4000");
        i3_log(LOG_MASK_ALWAYS, "    LOG_MASK_DEBUG              0x8000");
        i3_log(LOG_MASK_ALWAYS, "    LOG_MASK_TIMEOUT           0x10000");
#else
        // Logging is typically disabled to save space, so don't waste it with a bunch of printouts
        i3_log(LOG_MASK_WARN, "Log mask is of limited use, as logging is disabled");
#endif
    }
    else
    {
        i3_log_set_mask(lm);
        i3_log(LOG_MASK_ALWAYS, "The log mask is now 0x%x", lm);
#ifdef NO_REACH_LOGGING
        i3_log(LOG_MASK_WARN, "Log mask is of limited use, as logging is disabled");
#endif
    }
}

/* User code end [cli.c: User Local Functions] */

