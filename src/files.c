/********************************************************************************************
 *
 * \date   2024
 *
 * \author i3 Product Development (JNP)
 *
 * \brief  Functions to handle reading and writing files with Reach
 *
 ********************************************************************************************/

#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

#include "cr_stack.h"
#include "definitions.h"
#include "i3_log.h"

#include "const_files.h"
#include "fs_utils.h"

// If not defined, all files will just be stored in RAM.
// If pm_static.yml is removed, there will not be space in the file system for this and the param repo
#define FILES_USE_NVM_STORAGE

#define MAXIMUM_OTA_SIZE FLASH_AREA_SIZE(image_1)
#define OTA_RAM_BLOCK_SIZE 4096

#ifdef FILES_USE_NVM_STORAGE
#define IO_TXT_FILENAME "/lfs/io.txt"
#endif

#define MAX_IO_TXT_LENGTH 2048

static int ota_erase(void);
static int ota_write(const uint8_t *data, size_t offset, size_t size);
static int ota_flush_cache(void);
static int ota_mark_valid(void);
static int ota_write_helper(void);

static uint8_t ota_ram[OTA_RAM_BLOCK_SIZE];
static size_t ota_ram_start_offset = 0;
static size_t ota_ram_size = 0;

static char io_txt[MAX_IO_TXT_LENGTH];
static size_t io_txt_size = 0;

void files_init(void)
{
#ifdef FILES_USE_NVM_STORAGE
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
            memset(io_txt, 0, sizeof(io_txt));
            memcpy(io_txt, default_io_txt, sizeof(default_io_txt));
            io_txt_size = sizeof(default_io_txt);
            file_descriptions[FILE_IO_TXT].current_size_bytes = (int32_t) io_txt_size;
            break;
        }
        case 1:
        {
            // File exists, get it
            memset(io_txt, 0, sizeof(io_txt));
            io_txt_size = sizeof(io_txt);
            rval = fs_utils_get_file(IO_TXT_FILENAME, (uint8_t *) io_txt, 0, &io_txt_size);
            if (rval != 0)
                I3_LOG(LOG_MASK_ERROR, "io.txt read failed, error %d", rval);
            else
                file_descriptions[FILE_IO_TXT].current_size_bytes = (int32_t) io_txt_size;
            break;
        }
        default:
        {
            I3_LOG(LOG_MASK_ERROR, "io.txt access failed, error %d", rval);
            memset(io_txt, 0, sizeof(io_txt));
            memcpy(io_txt, default_io_txt, sizeof(default_io_txt));
            io_txt_size = sizeof(default_io_txt);
            file_descriptions[FILE_IO_TXT].current_size_bytes = (int32_t) io_txt_size;
            break;
        }
    }
#else
    file_descriptions[FILE_IO_TXT].storage_location = cr_StorageLocation_RAM;
    memset(io_txt, 0, sizeof(io_txt));
    memcpy(io_txt, default_io_txt, sizeof(default_io_txt));
    io_txt_size = sizeof(default_io_txt);
    file_descriptions[FILE_IO_TXT].current_size_bytes = (int32_t) io_txt_size;
#endif
}

void files_reset(void)
{
#ifdef FILES_USE_NVM_STORAGE
    int rval = fs_utils_update_file(IO_TXT_FILENAME, (uint8_t *) default_io_txt, sizeof(default_io_txt));
    if (rval != 0)
        I3_LOG(LOG_MASK_ERROR, "io.txt write failed, error %d", rval);
#endif
    memset(io_txt, 0, sizeof(io_txt));
    memcpy(io_txt, default_io_txt, sizeof(default_io_txt));
    io_txt_size = sizeof(default_io_txt);
    file_descriptions[FILE_IO_TXT].current_size_bytes = (int32_t) io_txt_size;
}

int ota_invalidate(void)
{
    return flash_erase(FLASH_AREA_DEVICE(image_1), FLASH_AREA_OFFSET(image_1) + FLASH_AREA_SIZE(image_1) - OTA_RAM_BLOCK_SIZE, OTA_RAM_BLOCK_SIZE);
}

int crcb_read_file(const uint32_t fid,          // which file
                 const int offset,              // offset, negative value specifies current location.
                 const size_t bytes_requested,  // how many bytes to read
                 uint8_t *pData,                // where the data goes
                 int *bytes_read)               // bytes actually read, negative for errors.
{
    if (bytes_requested > REACH_BYTES_IN_A_FILE_PACKET)
    {
        I3_LOG(LOG_MASK_ERROR, "%s: %d is more than the buffer for a file read (%d).",
               __FUNCTION__, fid, REACH_BYTES_IN_A_FILE_PACKET);
        return cr_ErrorCodes_BUFFER_TOO_SMALL;
    }
    switch (fid) {
        case FILE_IO_TXT:
            if (offset < 0 || offset >= io_txt_size)
                return cr_ErrorCodes_NO_DATA;
#ifdef FILES_USE_NVM_STORAGE
            if (offset == 0)
            {
                // Update the local buffer of the file in case a write failed before this read
                int rval = fs_utils_get_file(IO_TXT_FILENAME, (uint8_t *) io_txt, 0, &io_txt_size);
                if (rval != 0)
                    I3_LOG(LOG_MASK_ERROR, "io.txt read failed, error %d", rval);
            }
#endif
            *bytes_read = ((offset + bytes_requested) > io_txt_size) ? (io_txt_size - offset):bytes_requested;
            memcpy(pData, &io_txt[offset], (size_t) *bytes_read);
            break;
        case FILE_CYGNUS_REACH_LOGO_PNG:
            if (offset < 0 || offset >= sizeof(cygnus_reach_logo))
                return cr_ErrorCodes_NO_DATA;
            *bytes_read = ((offset + bytes_requested) > sizeof(cygnus_reach_logo)) ? (sizeof(cygnus_reach_logo) - offset):bytes_requested;
            memcpy(pData, &cygnus_reach_logo[offset], (size_t) *bytes_read);
            break;
        default:
            i3_log(LOG_MASK_ERROR, "Invalid file read (ID %u)", fid);
            return cr_ErrorCodes_BAD_FILE;
    }
    return 0;
}

int crcb_file_prepare_to_write(const uint32_t fid, const size_t offset, const size_t bytes)
{
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
            return rval;
        }
        case FILE_IO_TXT:
            // Partial writes are currently not supported by this demo
            if (offset != 0)
                return cr_ErrorCodes_INVALID_PARAMETER;
            if (offset + bytes > sizeof(io_txt))
                return cr_ErrorCodes_BUFFER_TOO_SMALL;
            memset(&io_txt[offset], 0, bytes);
            io_txt_size = bytes + offset;
            break;
        default:
            return cr_ErrorCodes_BAD_FILE;
    }
    return 0;
}

int crcb_write_file(const uint32_t fid, // which file
                 const int offset,      // offset, negative value specifies current location.
                 const size_t bytes,    // how many bytes to write
                 const uint8_t *pData)  // where to get the data from
{
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
            return rval;
        }
        case FILE_IO_TXT:
            if (offset < 0 || offset + bytes > io_txt_size)
                return cr_ErrorCodes_INVALID_PARAMETER;
            memcpy(&io_txt[offset], pData, bytes);
            break;
        default:
            return cr_ErrorCodes_BAD_FILE;
    }
    return 0;
}

int crcb_file_transfer_complete(const uint32_t fid)
{
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
            return rval;
        }
        case FILE_IO_TXT:
#ifdef FILES_USE_NVM_STORAGE
            int rval = fs_utils_update_file(IO_TXT_FILENAME, (uint8_t *) io_txt, io_txt_size);
            if (rval != 0)
                I3_LOG(LOG_MASK_ERROR, "io.txt write failed, error %d", rval);
#endif
            file_descriptions[FILE_IO_TXT].current_size_bytes = (int32_t) io_txt_size;
            break;
        default:
            return cr_ErrorCodes_BAD_FILE;
    }
    return 0;
}

// returns zero or an error code
int crcb_erase_file(const uint32_t fid)
{
    switch (fid)
    {
        case FILE_IO_TXT:
#ifdef FILES_USE_NVM_STORAGE
            int rval = fs_utils_erase_file(IO_TXT_FILENAME);
            if (rval != 0)
                I3_LOG(LOG_MASK_ERROR, "Failed to erase io.txt, error %d", rval);
#endif
            io_txt_size = 0;
            memset(io_txt, 0, sizeof(io_txt));
            file_descriptions[FILE_IO_TXT].current_size_bytes = (int32_t) io_txt_size;
            break;
        default:
            return cr_ErrorCodes_BAD_FILE;
    }
    return 0;
}

static int ota_erase(void)
{
    return flash_erase(FLASH_AREA_DEVICE(image_1), FLASH_AREA_OFFSET(image_1), FLASH_AREA_SIZE(image_1));
}

static int ota_write(const uint8_t *data, size_t offset, size_t size)
{
    int rval = 0;
    if (((offset + size) - ota_ram_start_offset) < OTA_RAM_BLOCK_SIZE)
    {
        // Case where everything will fit inside the cache and no write is required
        memcpy(&ota_ram[offset - ota_ram_start_offset], data, size);
        ota_ram_size = offset + size - ota_ram_start_offset;
    }
    else
    {
        // Cache overflow, a write will be required as well as probably filling up the cache partially
        if (offset - ota_ram_start_offset > OTA_RAM_BLOCK_SIZE)
        {
            // Special case where new write is totally outside the bounds of the current buffer
            rval = ota_write_helper();
            if (rval != 0)
                I3_LOG(LOG_MASK_ERROR, "Failed to clear OTA write cache, error %d", rval);
            // Align the start offset
            if (offset % 4)
                ota_ram_start_offset = offset - (offset % 4);
            else
                ota_ram_start_offset = offset;
            memcpy(&ota_ram[offset - ota_ram_start_offset], data, size);
            ota_ram_size = offset + size - ota_ram_start_offset;
        }
        else
        {
            // Write definitely requires flushing the cache, and may also require writing a small amount to the cache
            size_t write_1_size = ota_ram_start_offset + OTA_RAM_BLOCK_SIZE - offset;
            size_t write_2_size = size - write_1_size;
            memcpy(&ota_ram[offset - ota_ram_start_offset], data, write_1_size);
            ota_ram_size = OTA_RAM_BLOCK_SIZE;
            rval = ota_write_helper();
            if (rval != 0)
                I3_LOG(LOG_MASK_ERROR, "Failed to clear OTA write cache, error %d", rval);
            ota_ram_start_offset = offset + write_1_size;
            if (write_2_size > 0)
            {
                memcpy(ota_ram, &data[write_1_size], write_2_size);
                ota_ram_size = write_2_size;
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
    if (ota_ram_size % 4)
    {
        // Align to 4 bytes
        ota_ram_size += (4 - (ota_ram_size % 4));
    }
    int rval = flash_write(FLASH_AREA_DEVICE(image_1), FLASH_AREA_OFFSET(image_1) + ota_ram_start_offset, ota_ram, ota_ram_size);
    // Reset buffer
    ota_ram_size = 0;
    ota_ram_start_offset = 0;
    memset(ota_ram, 0, sizeof(ota_ram));
    return rval;
}