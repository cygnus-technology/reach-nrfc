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
 * \brief A minimal implementation of file discovery and read/write handling
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

#include "files.h"
#include "i3_error.h"
#include "i3_log.h"

/* User code start [files.c: User Includes] */
#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

#include "const_files.h"
#include "fs_utils.h"
/* User code end [files.c: User Includes] */

/********************************************************************************************
 *************************************     Defines     **************************************
 *******************************************************************************************/

/* User code start [files.c: User Defines] */
#define MAXIMUM_OTA_SIZE FLASH_AREA_SIZE(image_1)
#define OTA_RAM_BLOCK_SIZE 4096
#define IO_TXT_FILENAME "/lfs/io.txt"
#define MAX_IO_TXT_LENGTH 2048
/* User code end [files.c: User Defines] */

/********************************************************************************************
 ************************************     Data Types     ************************************
 *******************************************************************************************/

/* User code start [files.c: User Data Types] */
/* User code end [files.c: User Data Types] */

/********************************************************************************************
 *********************************     Global Variables     *********************************
 *******************************************************************************************/

/* User code start [files.c: User Global Variables] */
/* User code end [files.c: User Global Variables] */

/********************************************************************************************
 ***************************     Local Function Declarations     ****************************
 *******************************************************************************************/

static int sFindIndexFromFid(uint32_t fid, uint32_t *index);

/* User code start [files.c: User Local Function Declarations] */
static int ota_erase(void);
static int ota_write(const uint8_t *data, size_t offset, size_t size);
static int ota_flush_cache(void);
static int ota_mark_valid(void);
static int ota_write_helper(void);
/* User code end [files.c: User Local Function Declarations] */

/********************************************************************************************
 ******************************     Local/Extern Variables     ******************************
 *******************************************************************************************/

static int sFidIndex = 0;
cr_FileInfo sFileDescriptions[] = {
    {
        .file_id = FILE_OTA_BIN,
        .file_name = "ota.bin",
        .access = cr_AccessLevel_WRITE,
        .storage_location = cr_StorageLocation_NONVOLATILE,
        .require_checksum = false,
        .has_maximum_size_bytes = true,
        .maximum_size_bytes = 458240
    },
    {
        .file_id = FILE_IO_TXT,
        .file_name = "io.txt",
        .access = cr_AccessLevel_READ_WRITE,
        .storage_location = cr_StorageLocation_NONVOLATILE,
        .require_checksum = false,
        .has_maximum_size_bytes = true,
        .maximum_size_bytes = 4096
    },
    {
        .file_id = FILE_CYGNUS_REACH_LOGO_PNG,
        .file_name = "cygnus-reach-logo.png",
        .access = cr_AccessLevel_READ,
        .storage_location = cr_StorageLocation_NONVOLATILE,
        .require_checksum = false,
        .has_maximum_size_bytes = true,
        .maximum_size_bytes = 17900
    }
};

/* User code start [files.c: User Local/Extern Variables] */
static uint8_t sOtaRam[OTA_RAM_BLOCK_SIZE];
static size_t sOtaRamStartOffset = 0;
static size_t sOtaRamSize = 0;

static char sIoTxtContents[MAX_IO_TXT_LENGTH];
static size_t sIoTxtSize = 0;
/* User code end [files.c: User Local/Extern Variables] */

/********************************************************************************************
 *********************************     Global Functions     *********************************
 *******************************************************************************************/

void files_init(void)
{
    /* User code start [Files: Init] */

    // Create io.txt if it doesn't exist already
    int rval = fs_utils_file_exists(IO_TXT_FILENAME);
    switch (rval)
    {
        case 0:
        {
            // File doesn't exist yet, create it
            rval = fs_utils_update_file(IO_TXT_FILENAME, (uint8_t *) default_io_txt, sizeof(default_io_txt));
            if (rval != 0)
                I3_LOG(LOG_MASK_ERROR, "io.txt write failed, error %d", rval);
            memset(sIoTxtContents, 0, sizeof(sIoTxtContents));
            memcpy(sIoTxtContents, default_io_txt, sizeof(default_io_txt));
            sIoTxtSize = sizeof(default_io_txt);
            sFileDescriptions[FILE_IO_TXT].current_size_bytes = (int32_t) sIoTxtSize;
            break;
        }
        case 1:
        {
            // File exists, get it
            memset(sIoTxtContents, 0, sizeof(sIoTxtContents));
            sIoTxtSize = sizeof(sIoTxtContents);
            rval = fs_utils_get_file(IO_TXT_FILENAME, (uint8_t *) sIoTxtContents, 0, &sIoTxtSize);
            if (rval != 0)
                I3_LOG(LOG_MASK_ERROR, "io.txt read failed, error %d", rval);
            else
                sFileDescriptions[FILE_IO_TXT].current_size_bytes = (int32_t) sIoTxtSize;
            break;
        }
        default:
        {
            I3_LOG(LOG_MASK_ERROR, "io.txt access failed, error %d", rval);
            memset(sIoTxtContents, 0, sizeof(sIoTxtContents));
            memcpy(sIoTxtContents, default_io_txt, sizeof(default_io_txt));
            sIoTxtSize = sizeof(default_io_txt);
            sFileDescriptions[FILE_IO_TXT].current_size_bytes = (int32_t) sIoTxtSize;
            break;
        }
    }

    /* User code end [Files: Init] */
}

int files_set_description(uint32_t fid, cr_FileInfo *file_desc)
{
    int rval = 0;
    affirm(file_desc != NULL);
    uint32_t idx;
    rval = sFindIndexFromFid(fid, &idx);
    if (rval != 0)
        return rval;

    /* User code start [Files: Set Description]
     * If the file description needs to be updated (for example, changing the current size), now's the time */
    /* User code end [Files: Set Description] */

    sFileDescriptions[idx] = *file_desc;

    return rval;
}

/* User code start [files.c: User Global Functions] */

void files_reset(void)
{
    int rval = fs_utils_update_file(IO_TXT_FILENAME, (uint8_t *) default_io_txt, sizeof(default_io_txt));
    if (rval != 0)
        I3_LOG(LOG_MASK_ERROR, "io.txt write failed, error %d", rval);
    memset(sIoTxtContents, 0, sizeof(sIoTxtContents));
    memcpy(sIoTxtContents, default_io_txt, sizeof(default_io_txt));
    sIoTxtSize = sizeof(default_io_txt);
    sFileDescriptions[FILE_IO_TXT].current_size_bytes = (int32_t) sIoTxtSize;
}

int ota_invalidate(void)
{
    return flash_erase(FLASH_AREA_DEVICE(image_1), FLASH_AREA_OFFSET(image_1) + FLASH_AREA_SIZE(image_1) - OTA_RAM_BLOCK_SIZE, OTA_RAM_BLOCK_SIZE);
}

/* User code end [files.c: User Global Functions] */

/********************************************************************************************
 *************************     Cygnus Reach Callback Functions     **************************
 *******************************************************************************************/

int crcb_file_get_description(uint32_t fid, cr_FileInfo *file_desc)
{
    int rval = 0;
    affirm(file_desc != NULL);
    uint32_t idx;
    rval = sFindIndexFromFid(fid, &idx);
    if (rval != 0)
        return rval;

    /* User code start [Files: Get Description]
     * If the file description needs to be updated (for example, changing the current size), now's the time */
    /* User code end [Files: Get Description] */

    *file_desc = sFileDescriptions[idx];

    return rval;
}

int crcb_file_get_file_count()
{
    int i;
    int numAvailable = 0;
    for (i = 0; i < NUM_FILES; i++)
    {
        if (crcb_access_granted(cr_ServiceIds_FILES, sFileDescriptions[i].file_id)) numAvailable++;
    }
    return numAvailable;
}

int crcb_file_discover_reset(const uint8_t fid)
{
    int rval = 0;
    uint32_t idx;
    rval = sFindIndexFromFid(fid, &idx);
    if (0 != rval)
    {
        I3_LOG(LOG_MASK_ERROR, "%s(%d): invalid FID, using NUM_FILES.", __FUNCTION__, fid);
        sFidIndex = NUM_FILES;
        return cr_ErrorCodes_INVALID_ID;
    }
    if (!crcb_access_granted(cr_ServiceIds_FILES, sFileDescriptions[sFidIndex].file_id))
    {
        I3_LOG(LOG_MASK_ERROR, "%s(%d): Access not granted, using NUM_FILES.", __FUNCTION__, fid);
        sFidIndex = NUM_FILES;
        return cr_ErrorCodes_BAD_FILE;
    }
    sFidIndex = idx;
    return 0;
}

int crcb_file_discover_next(cr_FileInfo *file_desc)
{
    if (sFidIndex >= NUM_FILES) // end of search
        return cr_ErrorCodes_NO_DATA;

    while (!crcb_access_granted(cr_ServiceIds_FILES, file_desc[sFidIndex].file_id))
    {
        I3_LOG(LOG_MASK_FILES, "%s: sFidIndex (%d) skip, access not granted",
               __FUNCTION__, sFidIndex);
        sFidIndex++;
        if (sFidIndex >= NUM_FILES)
        {
            I3_LOG(LOG_MASK_PARAMS, "%s: skipped to sFidIndex (%d) >= NUM_FILES (%d)", __FUNCTION__, sFidIndex, NUM_FILES);
            return cr_ErrorCodes_NO_DATA;
        }
    }
    *file_desc = sFileDescriptions[sFidIndex++];
    return 0;
}

// which file
// offset, negative value specifies current location.
// how many bytes to read
// where the data goes
// bytes actually read, negative for errors.
int crcb_read_file(const uint32_t fid, const int offset, const size_t bytes_requested, uint8_t *pData, int *bytes_read)
{
    int rval = 0;
    uint32_t idx;
    rval = sFindIndexFromFid(fid, &idx);
    if (0 != rval)
    {
        I3_LOG(LOG_MASK_ERROR, "%s(%d): invalid FID.", __FUNCTION__, fid);
        return cr_ErrorCodes_INVALID_ID;
    }
    if (bytes_requested > REACH_BYTES_IN_A_FILE_PACKET)
    {
        I3_LOG(LOG_MASK_ERROR, "%s: %d is more than the buffer for a file read (%d).", __FUNCTION__, fid, REACH_BYTES_IN_A_FILE_PACKET);
        return cr_ErrorCodes_BUFFER_TOO_SMALL;
    }

    /* User code start [Files: Read]
     * The code generator does nothing to handle storing files, so this is where pData and bytes_read should be updated */

    switch (fid)
    {
        case FILE_IO_TXT:
            if (offset < 0 || offset >= sIoTxtSize)
                return cr_ErrorCodes_NO_DATA;
            if (offset == 0)
            {
                // Update the local buffer of the file in case a write failed before this read
                int rval = fs_utils_get_file(IO_TXT_FILENAME, (uint8_t *) sIoTxtContents, 0, &sIoTxtSize);
                if (rval != 0)
                    I3_LOG(LOG_MASK_ERROR, "io.txt read failed, error %d", rval);
            }
            *bytes_read = ((offset + bytes_requested) > sIoTxtSize) ? (sIoTxtSize - offset):bytes_requested;
            memcpy(pData, &sIoTxtContents[offset], (size_t) *bytes_read);
            break;
        case FILE_CYGNUS_REACH_LOGO_PNG:
            if (offset < 0 || offset >= sizeof(cygnus_reach_logo))
                return cr_ErrorCodes_NO_DATA;
            *bytes_read = ((offset + bytes_requested) > sizeof(cygnus_reach_logo)) ? (sizeof(cygnus_reach_logo) - offset):bytes_requested;
            memcpy(pData, &cygnus_reach_logo[offset], (size_t) *bytes_read);
            break;
    }

    /* User code end [Files: Read] */

    return rval;
}

int crcb_file_prepare_to_write(const uint32_t fid, const size_t offset, const size_t bytes)
{
    int rval = 0;
    uint32_t idx;
    rval = sFindIndexFromFid(fid, &idx);
    if (0 != rval)
    {
        I3_LOG(LOG_MASK_ERROR, "%s(%d): invalid FID.", __FUNCTION__, fid);
        return cr_ErrorCodes_INVALID_ID;
    }
    /* User code start [Files: Pre-Write]
     * This is the opportunity to prepare for a file write, or to reject it. */

    switch (fid)
    {
        case FILE_OTA_BIN:
        {
            if (offset != 0)
                return cr_ErrorCodes_INVALID_PARAMETER;
            if (bytes > MAXIMUM_OTA_SIZE)
            {
                I3_LOG(LOG_MASK_ERROR, "OTA file (size 0x%x) is larger than the allocated area in flash (size 0x%x)", bytes, MAXIMUM_OTA_SIZE);
                return cr_ErrorCodes_BUFFER_TOO_SMALL;
            }
            // Erase the flash space for OTA updates
            int rval = ota_erase();
            if (rval != 0)
            {
                I3_LOG(LOG_MASK_ERROR, "Flash erase failed, error %d", rval);
                rval = cr_ErrorCodes_WRITE_FAILED;
            }
            break;
        }
        case FILE_IO_TXT:
            // Partial writes are currently not supported by this demo
            if (offset != 0)
                return cr_ErrorCodes_INVALID_PARAMETER;
            if (offset + bytes > sizeof(sIoTxtContents))
                return cr_ErrorCodes_BUFFER_TOO_SMALL;
            memset(&sIoTxtContents[offset], 0, bytes);
            sIoTxtSize = bytes + offset;
            break;
    }

    /* User code end [Files: Pre-Write] */
    return rval;
}

// which file
// offset, negative value specifies current location.
// how many bytes to write
// where to get the data from
int crcb_write_file(const uint32_t fid, const int offset, const size_t bytes, const uint8_t *pData)
{
    int rval = 0;
    uint32_t idx;
    rval = sFindIndexFromFid(fid, &idx);
    if (0 != rval)
    {
        I3_LOG(LOG_MASK_ERROR, "%s(%d): invalid FID.", __FUNCTION__, fid);
        return cr_ErrorCodes_INVALID_ID;
    }
    /* User code start [Files: Write]
     * Here is where the received data should be copied to wherever the application is storing it */

    switch (fid)
    {
        case FILE_OTA_BIN:
        {
            int rval = ota_write(pData, offset, bytes);
            if (rval != 0)
            {
                I3_LOG(LOG_MASK_ERROR, "Failed to write OTA file at offset 0x%x, error %d", offset, rval);
                rval = cr_ErrorCodes_WRITE_FAILED;
            }
            break;
        }
        case FILE_IO_TXT:
            if (offset < 0 || offset + bytes > sIoTxtSize)
                return cr_ErrorCodes_INVALID_PARAMETER;
            memcpy(&sIoTxtContents[offset], pData, bytes);
            break;
    }

    /* User code end [Files: Write] */
    return rval;
}

int crcb_file_transfer_complete(const uint32_t fid)
{
    int rval = 0;
    uint32_t idx;
    rval = sFindIndexFromFid(fid, &idx);
    if (0 != rval)
    {
        I3_LOG(LOG_MASK_ERROR, "%s(%d): invalid FID.", __FUNCTION__, fid);
        return cr_ErrorCodes_INVALID_ID;
    }
    /* User code start [Files: Write Complete]
     * This allows the application to handle any actions which need to occur after a file has successfully been written */

    switch (fid)
    {
        case FILE_OTA_BIN:
        {
            int rval = ota_flush_cache();
            if (rval != 0)
            {
                I3_LOG(LOG_MASK_ERROR, "Failed to flush OTA cache, error %d", rval);
                return cr_ErrorCodes_WRITE_FAILED;
            }
            rval = ota_mark_valid();
            if (rval != 0)
            {
                I3_LOG(LOG_MASK_ERROR, "Failed to write magic number, error %d", rval);
                rval = cr_ErrorCodes_WRITE_FAILED;
            }
            break;
        }
        case FILE_IO_TXT:
            int rval = fs_utils_update_file(IO_TXT_FILENAME, (uint8_t *) sIoTxtContents, sIoTxtSize);
            if (rval != 0)
                I3_LOG(LOG_MASK_ERROR, "io.txt write failed, error %d", rval);
            sFileDescriptions[FILE_IO_TXT].current_size_bytes = (int32_t) sIoTxtSize;
            break;
    }

    /* User code end [Files: Write Complete] */
    return rval;
}

// returns zero or an error code
int crcb_erase_file(const uint32_t fid)
{
    int rval = 0;
    uint32_t idx;
    rval = sFindIndexFromFid(fid, &idx);
    if (0 != rval)
    {
        I3_LOG(LOG_MASK_ERROR, "%s(%d): invalid FID.", __FUNCTION__, fid);
        return cr_ErrorCodes_INVALID_ID;
    }
    /* User code start [Files: Erase]
     * The exact meaning of "erasing" is user-defined, depending on how files are stored by the application */

    switch (fid)
    {
        case FILE_IO_TXT:
            int rval = fs_utils_erase_file(IO_TXT_FILENAME);
            if (rval != 0)
                I3_LOG(LOG_MASK_ERROR, "Failed to erase io.txt, error %d", rval);
            sIoTxtSize = 0;
            memset(sIoTxtContents, 0, sizeof(sIoTxtContents));
            sFileDescriptions[FILE_IO_TXT].current_size_bytes = (int32_t) sIoTxtSize;
            break;
    }

    /* User code end [Files: Erase] */
    return rval;
}

/* User code start [files.c: User Cygnus Reach Callback Functions] */
/* User code end [files.c: User Cygnus Reach Callback Functions] */

/********************************************************************************************
 *********************************     Local Functions     **********************************
 *******************************************************************************************/

static int sFindIndexFromFid(uint32_t fid, uint32_t *index)
{
    uint32_t idx;
    for (idx = 0; idx < NUM_FILES; idx++)
    {
        if (sFileDescriptions[idx].file_id == fid)
        {
            *index = idx;
            return 0;
        }
    }
    return cr_ErrorCodes_INVALID_ID;
}

/* User code start [files.c: User Local Functions] */

static int ota_erase(void)
{
    return flash_erase(FLASH_AREA_DEVICE(image_1), FLASH_AREA_OFFSET(image_1), FLASH_AREA_SIZE(image_1));
}

static int ota_write(const uint8_t *data, size_t offset, size_t size)
{
    int rval = 0;
    if (((offset + size) - sOtaRamStartOffset) < OTA_RAM_BLOCK_SIZE)
    {
        // Case where everything will fit inside the cache and no write is required
        memcpy(&sOtaRam[offset - sOtaRamStartOffset], data, size);
        sOtaRamSize = offset + size - sOtaRamStartOffset;
    }
    else
    {
        // Cache overflow, a write will be required as well as probably filling up the cache partially
        if (offset - sOtaRamStartOffset > OTA_RAM_BLOCK_SIZE)
        {
            // Special case where new write is totally outside the bounds of the current buffer
            rval = ota_write_helper();
            if (rval != 0)
                I3_LOG(LOG_MASK_ERROR, "Failed to clear OTA write cache, error %d", rval);
            // Align the start offset
            if (offset % 4)
                sOtaRamStartOffset = offset - (offset % 4);
            else
                sOtaRamStartOffset = offset;
            memcpy(&sOtaRam[offset - sOtaRamStartOffset], data, size);
            sOtaRamSize = offset + size - sOtaRamStartOffset;
        }
        else
        {
            // Write definitely requires flushing the cache, and may also require writing a small amount to the cache
            size_t write_1_size = sOtaRamStartOffset + OTA_RAM_BLOCK_SIZE - offset;
            size_t write_2_size = size - write_1_size;
            memcpy(&sOtaRam[offset - sOtaRamStartOffset], data, write_1_size);
            sOtaRamSize = OTA_RAM_BLOCK_SIZE;
            rval = ota_write_helper();
            if (rval != 0)
                I3_LOG(LOG_MASK_ERROR, "Failed to clear OTA write cache, error %d", rval);
            sOtaRamStartOffset = offset + write_1_size;
            if (write_2_size > 0)
            {
                memcpy(sOtaRam, &data[write_1_size], write_2_size);
                sOtaRamSize = write_2_size;
            }
        }
    }
    return rval;
}

static int ota_flush_cache(void)
{
    return ota_write_helper();
}

static int ota_mark_valid(void)
{
    uint8_t magic_number[] = {0x77, 0xc2, 0x95, 0xf3, 0x60, 0xd2, 0xef, 0x7f, 0x35, 0x52 , 0x50, 0x0f, 0x2c, 0xb6, 0x79, 0x80};
    return flash_write(FLASH_AREA_DEVICE(image_1), FLASH_AREA_OFFSET(image_1) + FLASH_AREA_SIZE(image_1) - sizeof(magic_number), magic_number, sizeof(magic_number));
}

static int ota_write_helper(void)
{
    if (sOtaRamSize % 4)
    {
        // Align to 4 bytes
        sOtaRamSize += (4 - (sOtaRamSize % 4));
    }
    int rval = flash_write(FLASH_AREA_DEVICE(image_1), FLASH_AREA_OFFSET(image_1) + sOtaRamStartOffset, sOtaRam, sOtaRamSize);
    // Reset buffer
    sOtaRamSize = 0;
    sOtaRamStartOffset = 0;
    memset(sOtaRam, 0, sizeof(sOtaRam));
    return rval;
}

/* User code end [files.c: User Local Functions] */

