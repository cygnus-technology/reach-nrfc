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
 * \brief A minimal implementation of device info discovery
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

#include "device.h"
#include "app_version.h"
#include "cr_stack.h"
#include "i3_log.h"

/* User code start [device.c: User Includes] */
/* User code end [device.c: User Includes] */

/********************************************************************************************
 *************************************     Defines     **************************************
 *******************************************************************************************/

#define TOSTRING_LAYER_ONE(x) #x
#define TOSTRING(x) TOSTRING_LAYER_ONE(x)

#ifdef DEV_BUILD
#define APP_VERSION_TAIL "-dev"
#else
#define APP_VERSION_TAIL ""
#endif // DEV_BUILD

/* User code start [device.c: User Defines] */
/* User code end [device.c: User Defines] */

/********************************************************************************************
 ************************************     Data Types     ************************************
 *******************************************************************************************/

/* User code start [device.c: User Data Types] */
/* User code end [device.c: User Data Types] */

/********************************************************************************************
 *********************************     Global Variables     *********************************
 *******************************************************************************************/

/* User code start [device.c: User Global Variables] */
/* User code end [device.c: User Global Variables] */

/********************************************************************************************
 ***************************     Local Function Declarations     ****************************
 *******************************************************************************************/

/* User code start [device.c: User Local Function Declarations] */
/* User code end [device.c: User Local Function Declarations] */

/********************************************************************************************
 ******************************     Local/Extern Variables     ******************************
 *******************************************************************************************/

static const char sAppVersion[] = TOSTRING(APP_MAJOR_VERSION) "." TOSTRING(APP_MINOR_VERSION) "." TOSTRING(APP_PATCH_VERSION) APP_VERSION_TAIL;
const cr_DeviceInfoResponse sDeviceInfo = {
    .device_name = "nRF52840 Dongle",
    .manufacturer = "Nordic Semiconductor",
    .device_description = "A demo of Reach features",
    .services = cr_ServiceIds_PARAMETER_REPO | cr_ServiceIds_FILES | cr_ServiceIds_COMMANDS | cr_ServiceIds_CLI | cr_ServiceIds_TIME
};

/* User code start [device.c: User Local/Extern Variables] */
/* User code end [device.c: User Local/Extern Variables] */

/********************************************************************************************
 *********************************     Global Functions     *********************************
 *******************************************************************************************/

const char *get_app_version()
{
    return sAppVersion;
}

/* User code start [device.c: User Global Functions] */
/* User code end [device.c: User Global Functions] */

/********************************************************************************************
 *************************     Cygnus Reach Callback Functions     **************************
 *******************************************************************************************/

// The stack will call this function.
// The const copy of the basis in flash is copied to RAM so that the device
// can overwrite varying data like SN and hash.
int crcb_device_get_info(const cr_DeviceInfoRequest *request, cr_DeviceInfoResponse *pDi)
{
    (void) request;
    memcpy(pDi, &sDeviceInfo, sizeof(cr_DeviceInfoResponse));
    I3_LOG(LOG_MASK_REACH, "%s: %s\n", __FUNCTION__, sDeviceInfo.device_name);

    sprintf(pDi->firmware_version, "%s", sAppVersion);

    /* User code start [Device: Get Info]
     * Here, further modifications can be made to the contents of pDi if needed */
    /* User code end [Device: Get Info] */

    return 0;
}

/* User code start [device.c: User Cygnus Reach Callback Functions] */
/* User code end [device.c: User Cygnus Reach Callback Functions] */

/********************************************************************************************
 *********************************     Local Functions     **********************************
 *******************************************************************************************/

/* User code start [device.c: User Local Functions] */
/* User code end [device.c: User Local Functions] */

