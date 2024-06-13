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
 * \brief A minimal implementation of command discovery and handling
 *
 * Original Author: Chuck Peplinski
 * Script Authors: Joseph Peplinski and Andrew Carlson
 *
 * Generated with version 1.0.0 of the C code generator
 *
 ********************************************************************************************/

#ifndef _COMMANDS_H_
#define _COMMANDS_H_

// Includes
/* User code start [commands.h: User Includes] */
/* User code end [commands.h: User Includes] */

// Defines
#define NUM_COMMANDS 6

/* User code start [commands.h: User Defines] */
/* User code end [commands.h: User Defines] */

// Data Types
typedef enum {
    COMMAND_PRESET_NOTIFICATIONS_ON,
    COMMAND_CLEAR_NOTIFICATIONS,
    COMMAND_REBOOT,
    COMMAND_RESET_DEFAULTS,
    COMMAND_INVALIDATE_OTA_IMAGE,
    COMMAND_CLICK_FOR_WISDOM,
} command_t;

/* User code start [commands.h: User Data Types] */
/* User code end [commands.h: User Data Types] */

// Global Variables
/* User code start [commands.h: User Global Variables] */
/* User code end [commands.h: User Global Variables] */

// Global Functions
/* User code start [commands.h: User Global Functions] */
/* User code end [commands.h: User Global Functions] */


#endif // _COMMANDS_H_