/********************************************************************************************
 *
 * \date   2024
 *
 * \author i3 Product Development (JNP)
 *
 * \brief  Utilities to simplify common file access patterns
 *
 ********************************************************************************************/

#include "fs_utils.h"

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

/*******************************************************************************
 ****************************   LOCAL  TYPES   *********************************
 ******************************************************************************/

/*******************************************************************************
 *********************   LOCAL FUNCTION PROTOTYPES   ***************************
 ******************************************************************************/

/*******************************************************************************
 ***************************  LOCAL VARIABLES   ********************************
 ******************************************************************************/

static bool out_of_space = false;

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

int fs_utils_file_exists(char *path)
{
    struct fs_dirent entry;
    int rval = fs_stat(path, &entry);
    switch (rval)
    {
        case 0:
            return 1;
        case -ENOENT:
            return 0;
        default:
            return rval;
    }
}

int fs_utils_get_file_size(char *path)
{
    struct fs_dirent entry;
    int rval = fs_stat(path, &entry);
    switch (rval)
    {
        case 0:
            return (int) entry.size;
        case -ENOENT:
            return 0;
        default:
            return rval;
    }
}

int fs_utils_get_file(char *fname, uint8_t *data, int32_t offset, size_t *size)
{
    int exists = fs_utils_file_exists(fname);
    switch (exists)
    {
        case 0:
            return -2;
        case 1:
            // Fine to continue
            break;
        default:
            return -1;
    }
    int32_t file_size = fs_utils_get_file_size(fname);
    if (file_size <= 0)
    {
        return -3;
    }
    if (offset > file_size)
    {
        return -4;
    }

    struct fs_file_t file;
    fs_file_t_init(&file);
    int rval = fs_open(&file, fname, FS_O_READ);
    switch (rval)
    {
        case 0:
            break;
        default:
            return -5;
    }
    if (offset > 0)
    {
        rval = fs_seek(&file, offset, FS_SEEK_SET);
        if (rval != 0)
        {
            return -6;
        }
    }
    int r_size = fs_read(&file, data, *size);
    rval = fs_close(&file);
    if (r_size < 0)
    {
        return -7;
    }
    *size = r_size;
    return (rval != 0) ? -8:0;
}

int fs_utils_update_file(char *fname, uint8_t *data, size_t size)
{
    // Check if the file exists already
    int rval = fs_utils_file_exists(fname);
    switch (rval)
    {
        case 0:
            // Nothing to erase
            if (out_of_space)
            {
                // No way this will work until something gets erased
                return -5;
            }
            break;
        case 1:
            // Erase the current file
            rval = fs_utils_erase_file(fname);
            if (rval != 0)
                return -2;
            break;
        default:
            return -1;
    }

    struct fs_file_t file;
    fs_file_t_init(&file);
    rval = fs_open(&file, fname, FS_O_CREATE | FS_O_RDWR);
    switch (rval)
    {
        case 0:
            break;
        case -EINVAL:
            return -3;
        default:
            return -4;
    }
    ssize_t r_size = -1;
    r_size = fs_write(&file, data, size);
    rval = fs_close(&file);
    if (r_size != size)
    {
        if (r_size == -28)
        {
            // Out of space
            out_of_space = true;
            return -5;
        }
        return -6;
    }
    else
    {
        return (rval != 0) ? -7:0;
    }
}

int fs_utils_erase_file(char *path)
{
    int rval = fs_utils_file_exists(path);
    switch (rval)
    {
        case 0:
            // No file to erase
            break;
        case 1:
            // Erase file
            rval = fs_unlink(path);
            if (out_of_space)
            {
                out_of_space = false;
            }
            break;
        default:
            return -1;
    }
    return rval;
}