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
 * \brief A (user-defined) implementation of the time service
 *
 * Original Author: Chuck Peplinski
 * Script Authors: Joseph Peplinski and Andrew Carlson
 *
 * Generated with version 1.0.0 of the C code generator
 *
 ********************************************************************************************/

/********************************************************************************************
 ************************************     Includes     *************************************
 *******************************************************************************************/

#include "cr_stack.h"

/* User code start [time.c: User Includes] */

#include <zephyr/posix/time.h>
#include "parameters.h"

/* User code end [time.c: User Includes] */

/********************************************************************************************
 *************************************     Defines     **************************************
 *******************************************************************************************/

/* User code start [time.c: User Defines] */
/* User code end [time.c: User Defines] */

/********************************************************************************************
 ***********************************     Data Types     ************************************
 *******************************************************************************************/

/* User code start [time.c: User Data Types] */
/* User code end [time.c: User Data Types] */

/********************************************************************************************
 ********************************     Global Variables     *********************************
 *******************************************************************************************/

/* User code start [time.c: User Global Variables] */
/* User code end [time.c: User Global Variables] */

/********************************************************************************************
 *****************************     Local/Extern Variables     ******************************
 *******************************************************************************************/

/* User code start [time.c: User Local/Extern Variables] */

static int64_t time_offset = 0;

/* User code end [time.c: User Local/Extern Variables] */

/********************************************************************************************
 ***************************     Local Function Declarations     ****************************
 *******************************************************************************************/

/* User code start [time.c: User Local Function Declarations] */
/* User code end [time.c: User Local Function Declarations] */

/********************************************************************************************
 ********************************     Global Functions     *********************************
 *******************************************************************************************/

/* User code start [time.c: User Global Functions] */
/* User code end [time.c: User Global Functions] */

/********************************************************************************************
 *************************     Cygnus Reach Callback Functions     **************************
 *******************************************************************************************/

int crcb_time_get(cr_TimeGetResponse *response)
{
    /* User code start [Time: Get] */

    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    response->seconds_utc = (int64_t) now.tv_sec;
    cr_ParameterValue data;
    int rval = crcb_parameter_read(PARAM_TIMEZONE_ENABLED, &data);
    if (rval)
        return cr_ErrorCodes_READ_FAILED;
    response->has_timezone = data.value.bool_value;
    rval = crcb_parameter_read(PARAM_TIMEZONE_OFFSET, &data);
    if (rval)
        return cr_ErrorCodes_READ_FAILED;
    if (!response->has_timezone)
    {
        response->seconds_utc += data.value.int32_value;
    }
    else
    {
        response->timezone = data.value.int32_value;
    }

    /* User code end [Time: Get] */
    return 0;
}

int crcb_time_set(const cr_TimeSetRequest *request)
{
    /* User code start [Time: Set] */

    struct timespec time = {.tv_sec = (time_t) request->seconds_utc};
    clock_settime(CLOCK_REALTIME, &time);
    if (request->has_timezone)
    {
        cr_ParameterValue param;
        param.parameter_id = PARAM_TIMEZONE_OFFSET;
        param.which_value = cr_ParameterValue_int32_value_tag;
        param.timestamp = k_uptime_get_32();
        param.value.int32_value = request->timezone;
        crcb_parameter_write(PARAM_TIMEZONE_OFFSET, &param);
    }

    /* User code end [Time: Set] */
    return 0;
}

/* User code start [time.c: User Cygnus Reach Callback Functions] */
/* User code end [time.c: User Cygnus Reach Callback Functions] */

/********************************************************************************************
 *********************************     Local Functions     **********************************
 *******************************************************************************************/

/* User code start [time.c: User Local Functions] */
/* User code end [time.c: User Local Functions] */

