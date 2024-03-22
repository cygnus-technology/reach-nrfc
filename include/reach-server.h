/********************************************************************************************
 *    _ ____  ___             _         _     ___              _                        _
 *   (_)__ / | _ \_ _ ___  __| |_  _ __| |_  |   \ _____ _____| |___ _ __ _ __  ___ _ _| |_
 *   | ||_ \ |  _/ '_/ _ \/ _` | || / _|  _| | |) / -_) V / -_) / _ \ '_ \ '  \/ -_) ' \  _|
 *   |_|___/ |_| |_| \___/\__,_|\_,_\__|\__| |___/\___|\_/\___|_\___/ .__/_|_|_\___|_||_\__|
 *                                                                  |_|
 *                           -----------------------------------
 *                          Copyright i3 Product Development 2023
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
 * \brief This file specifies aspects of the Reach server that are controlled by the app.
 *
 * Original Author: Chuck.Peplinski
 *
 ********************************************************************************************/

#ifndef _REACH_SERVER_H_
#define _REACH_SERVER_H_

#include "definitions.h"

// A range of parameters are driven by the 244 byte packet size imposed by BLE.


// As the app is using BLE, the largest encoded buffer cannot be larger than 
// 244 bytes.  The Reach stack will statically allocate two buffers of this size, 
// for encoding and decoding
#define CR_CODED_BUFFER_SIZE    244

// The raw data that encodes to BLE might be slightly larger.
// The Reach stack will allocate one buffer of this size, for decoding the prompt.
// The app is to provide the raw memory to be encoded.
#define CR_DECODED_BUFFER_SIZE   256

#define APP_ADVERTISED_NAME_LENGTH 27

// Define this to enable remote CLI via a buffer.
#define ENABLE_REMOTE_CLI
#ifdef ENABLE_REMOTE_CLI
  // If false use a command to enable it.
  #define REMOTE_CLI_ECHO_ON_DEFAULT    true
#endif

// Use this to set the default logging level.  0x0 will only report high-priority text and warnings/errors
#define DEFAULT_LOG_MASK 0x0

// Define this to be a 32 bit number that must be provided to access the device.
// #define APP_REQUIRED_CHALLENGE_KEY  0x1020304

// Define this to be a 32 bit number that must be provided to access a longer
// list of parameters.
// #define APP_REQUIRED_PARAMETER_KEY  0x9080706

// Setting this to zero removes support for unpolled parameter change notification
// Defines the size of the array holding param notification specifications.
#define NUM_SUPPORTED_PARAM_NOTIFY  NUM_PARAMS

// Configuration of error reporting by cr_report_error()
#define ERROR_FORMAT_LOG_ONLY   0  // only log to console
#define ERROR_FORMAT_SHORT      1  // send error code only to client
#define ERROR_FORMAT_FULL       2  // send error message to client.
#define ERROR_REPORT_FORMAT     ERROR_FORMAT_FULL

#include "reach.pb.h"

// Ideally all of the buffer sizes flow from here.
#include "reach_ble_proto_sizes.h"

// This is the number of buffers allocated.
// It's coded here to be the same as the number allowed in a transfer.
// With code changes it could be less.
#define REACH_PARAM_BUFFER_COUNT REACH_COUNT_PARAM_IDS

#endif  // ndef _REACH_SERVER_H_