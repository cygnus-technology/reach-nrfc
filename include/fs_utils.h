/********************************************************************************************
 *
 * \date   2024
 *
 * \author i3 Product Development (JNP)
 *
 * \brief  Utilities to simplify common file access patterns
 *
 ********************************************************************************************/

#include <stdint.h>
#include <stddef.h>

/**
 * Checks if the specified file exists on the file system
 * @param path The path of the file to look for
 * @return 1 if the file exists, 0 if the file does not exist, or a negative error code on file system error
 */
int fs_utils_file_exists(char *path);

/**
 * Gets the file size of the requested file
 * @param path The path of the file to look for
 * @return The size of the file if successful, 0 if the file does not exist, or a negative error code on file system error
 */
int fs_utils_get_file_size(char *path);

/**
 * Gets the contents of the requested file (if it exists)
 * @param path The path of the file to look for
 * @param data A pointer to the array to fill with the file contents
 * @param offset Where to start reading the file from
 * @param size A pointer to the size of the provided array, which will be updated to reflect the size of the file contents
 * @return 0 on success, or a negative error code on failure (including if the file does not exist)
 */
int fs_utils_get_file(char *fname, uint8_t *data, int32_t offset, size_t *size);

/**
 * Writes a new file to the specified path with the provided contents, erasing any existing file at that path
 * @param path The path of the file to create
 * @param data A pointer to the new file contents
 * @param size The size of the new file contents
 * @return 0 on success, or a negative error code on failure
 */
int fs_utils_update_file(char *fname, uint8_t *data, size_t size);

/**
 * Erases the specified file
 * @param path The path of the file to erase
 * @return 0 on success (including if the file did not exist), or a negative error code on failure
 */
int fs_utils_erase_file(char *fname);