/********************************************************************************************
 *
 * \date   2023
 *
 * \author i3 Product Development (JNP)
 *
 * \brief  Functions to handle reading and writing parameters with Reach
 *
 ********************************************************************************************/

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/fs/fs.h>
#include <zephyr/posix/time.h>

#include "cr_stack.h"
#include "definitions.h"
#include "i3_log.h"
#include "reach_nrf_connect.h"

#include "main.h"
#include "fs_utils.h"

#define PARAM_REPO_USE_FILE_STORAGE

#ifdef PARAM_REPO_USE_FILE_STORAGE
#define PARAM_REPO_FILE "/lfs/pr"

// A custom hash function to only look at NVM parameters, to avoid invalidating the PR file when only non-NVM parameters change
static uint32_t calculate_nvm_hash(void);

static bool pr_file_access_failed = false;
static bool pr_file_exists = false;
static struct fs_file_t pr_file;

static uint32_t nvm_param_ids[NUM_PARAMS];
static uint16_t nvm_param_count = 0;
#endif

int param_repo_reset_nvm(void)
{
    for (uint16_t i = 0; i < NUM_PARAMS; i++)
    {
        if (param_desc[i].storage_location != cr_StorageLocation_NONVOLATILE)
            continue;
        cr_ParameterValue param;
        param.parameter_id = sCr_param_val[i].parameter_id;
        param.which_value = sCr_param_val[param.parameter_id].which_value;
        param.timestamp = k_uptime_get_32();
        I3_LOG(LOG_MASK_ALWAYS, "Resetting ID %u, type %u", param.parameter_id, param.which_value);
        // Fill in default value if it's defined
        switch (param.which_value)
        {
            case cr_ParameterValue_uint32_value_tag:
                if (param_desc[param.parameter_id].has_default_value)
                    param.value.uint32_value = (uint32_t)param_desc[param.parameter_id].default_value;
                else
                    param.value.uint32_value = 0;
                break;
            case cr_ParameterValue_sint32_value_tag:
                if (param_desc[param.parameter_id].has_default_value)
                    param.value.sint32_value = (int32_t)param_desc[param.parameter_id].default_value;
                else
                    param.value.sint32_value = 0;
                break;
            case cr_ParameterValue_float32_value_tag:
                if (param_desc[param.parameter_id].has_default_value)
                    param.value.float32_value = (float)param_desc[param.parameter_id].default_value;
                else
                    param.value.float32_value = 0;
                break;
            case cr_ParameterValue_uint64_value_tag:
                if (param_desc[param.parameter_id].has_default_value)
                    param.value.uint64_value = (uint64_t)param_desc[param.parameter_id].default_value;
                else
                    param.value.uint64_value = 0;
                break;
            case cr_ParameterValue_sint64_value_tag:
                if (param_desc[param.parameter_id].has_default_value)
                    param.value.sint64_value = (int64_t)param_desc[param.parameter_id].default_value;
                else
                    param.value.sint64_value = 0;
                break;
            case cr_ParameterValue_float64_value_tag:
                if (param_desc[param.parameter_id].has_default_value)
                    param.value.float64_value = param_desc[param.parameter_id].default_value;
                else
                    param.value.float64_value = 0;
                break;
            case cr_ParameterValue_bool_value_tag:
                if (param_desc[param.parameter_id].has_default_value)
                    param.value.bool_value = (bool)param_desc[param.parameter_id].default_value;
                else
                    param.value.bool_value = false;
                break;
            case cr_ParameterValue_enum_value_tag:
                if (param_desc[param.parameter_id].has_default_value)
                    param.value.enum_value = (uint32_t)param_desc[param.parameter_id].default_value;
                else
                    param.value.enum_value = 0;
                break;
            case cr_ParameterValue_bitfield_value_tag:
                if (param_desc[param.parameter_id].has_default_value)
                    param.value.bitfield_value = (uint32_t)param_desc[param.parameter_id].default_value;
                else
                    param.value.bitfield_value = 0;
                break;
            case cr_ParameterValue_bytes_value_tag:
                // Default value does not exist, but there is a default size
                param.value.bytes_value.size  = param_desc[param.parameter_id].size_in_bytes;
                memset(param.value.bytes_value.bytes, 0, sizeof(param.value.bytes_value.bytes));
                break;
            case cr_ParameterValue_string_value_tag:
                // No initialization necessary
                memset(param.value.string_value, 0, sizeof(param.value.string_value));
                break;
            default:
                affirm(0);  // should not happen.
                break;
        }
        int rval = crcb_parameter_write(param.parameter_id, &param);
        if (rval)
        {
            I3_LOG(LOG_MASK_ERROR, "Failed to reset parameter '%s', error %d", param_desc[i].name, rval);
            return rval;
        }
    }
    return 0;
}

int app_handle_param_repo_pre_init(void)
{
#ifdef PARAM_REPO_USE_FILE_STORAGE
    int rval = 0;
    fs_file_t_init(&pr_file);
    uint32_t hash = calculate_nvm_hash();
    uint32_t stored_hash = 0;
    if (fs_utils_file_exists(PARAM_REPO_FILE) == 1)
    {
        I3_LOG(LOG_MASK_PARAMS, "PR file found");
        // Verify that the file is valid and isn't for an earlier version of parameter repo
        // Theoretically should only cover parameters in the NVM, but this gets complicated with enum/bitfield descriptions tied to parameters
        rval = fs_open(&pr_file, PARAM_REPO_FILE, FS_O_RDWR);
        if (rval < 0)
        {
            I3_LOG(LOG_MASK_ERROR, "Failed to open PR file, error %d", rval);
            pr_file_access_failed = true;
            return -1;
        }
        rval = (int) fs_read(&pr_file, &stored_hash, sizeof(stored_hash));
        if (rval < sizeof(stored_hash) || stored_hash != hash)
        {
            if (rval < sizeof(stored_hash))
                I3_LOG(LOG_MASK_ERROR, "PR file read returned %d, expected %u", rval, sizeof(uint32_t));
            else
                I3_LOG(LOG_MASK_WARN, "Stored hash 0x%x does not match computed hash 0x%x", stored_hash, hash);
            // Close and erase the file, and continue as if the file never existed
            rval = fs_close(&pr_file);
            if (rval < 0)
            {
                I3_LOG(LOG_MASK_ERROR, "Failed to close invalid PR file, error %d", rval);
                pr_file_access_failed = true;
                return -2;
            }
            rval = fs_unlink(PARAM_REPO_FILE);
            if (rval < 0)
            {
                I3_LOG(LOG_MASK_ERROR, "Failed to unlink invalid PR file, error %d", rval);
                pr_file_access_failed = true;
                return -3;
            }
        }
        else
        {
            pr_file_exists = true;
        }
    }
    if (!pr_file_exists)
    {
        I3_LOG(LOG_MASK_WARN, "No valid PR file found, creating a new one");
        rval = fs_open(&pr_file, PARAM_REPO_FILE, FS_O_RDWR | FS_O_CREATE);
        if (rval < 0)
        {
            I3_LOG(LOG_MASK_ERROR, "Failed to create PR file, error %d", rval);
            pr_file_access_failed = true;
            return -4;
        }
        stored_hash = ~hash;
        rval = (int) fs_write(&pr_file, &stored_hash, sizeof(stored_hash));
        if (rval < 0)
        {
            I3_LOG(LOG_MASK_ERROR, "Failed to write anti-hash to PR file, error %d", rval);
            pr_file_access_failed = true;
            return -5;
        }
    }
#endif // PARAM_REPO_USE_FILE_STORAGE
    return 0;
}

int app_handle_param_repo_init(cr_ParameterValue *data, cr_ParameterInfo *desc)
{
    int rval = 0;
#ifdef PARAM_REPO_USE_FILE_STORAGE
    if (desc->storage_location == cr_StorageLocation_NONVOLATILE)
    {
        if (!pr_file_access_failed)
        {
            if (pr_file_exists)
            {
                I3_LOG(LOG_MASK_PARAMS, "Getting data about parameter %u from existing PR file", data->parameter_id);
                // Get the data from the file
                cr_ParameterInfo temp;
                rval = (int) fs_read(&pr_file, &temp, sizeof(cr_ParameterValue));
                if (rval != sizeof(cr_ParameterValue))
                {
                    I3_LOG(LOG_MASK_ERROR, "Failed to read parameter ID %u, error %d", data->parameter_id, rval);
                    pr_file_access_failed = true;
                }
                else
                {
                    rval = 0;
                }
                // If we got the data successfully, copy it to the parameter
                memcpy(data, &temp, sizeof(temp));
                // Update parameter map
                nvm_param_ids[nvm_param_count++] = data->parameter_id;
            }
            else
            {
                I3_LOG(LOG_MASK_PARAMS, "Writing data about parameter %u to new PR file", data->parameter_id);
                nvm_param_ids[nvm_param_count++] = data->parameter_id;
                // Write the data to the file
                rval = (int) fs_write(&pr_file, data, sizeof(cr_ParameterValue));
                if (rval != sizeof(cr_ParameterValue))
                {
                    I3_LOG(LOG_MASK_ERROR, "Failed to write parameter ID %u, error %d", data->parameter_id, rval);
                    pr_file_access_failed = true;
                }
                else
                {
                    rval = 0;
                }
            }
        }
    }
#endif // PARAM_REPO_USE_FILE_STORAGE

    switch (data->parameter_id)
    {
        case PARAM_IDENTIFY_INTERVAL:
            rval = app_handle_param_repo_read(data);
            main_set_identify_interval(data->value.float32_value);
            break;
        default:
            // Call the standard read function
            rval = app_handle_param_repo_read(data);
            break;
    }
    return rval;
}

int app_handle_param_repo_post_init(void)
{
#ifdef PARAM_REPO_USE_FILE_STORAGE
    if (!pr_file_access_failed)
    {
        if (!pr_file_exists)
        {
            I3_LOG(LOG_MASK_PARAMS, "Marking fresh PR file as valid");
            // Mark as valid
            fs_seek(&pr_file, 0, FS_SEEK_SET);
            uint32_t hash = calculate_nvm_hash();
            fs_write(&pr_file, &hash, sizeof(uint32_t));
        }
        fs_close(&pr_file);
    }
    else
    {
        // Depending on the source of this failure, this might not work, but it's important to ensure there's no corrupted file in the NVM
        fs_close(&pr_file);
        fs_unlink(PARAM_REPO_FILE);
        return -1;
    }
#endif // PARAM_REPO_USE_FILE_STORAGE
    return 0;
}

int app_handle_param_repo_read(cr_ParameterValue *data)
{
    int rval = 0;
    switch (data->parameter_id)
    {
        // Parameters which may change without the param repo's knowledge
        case PARAM_BT_DEVICE_ADDRESS:
            bt_addr_le_t ble_id;
            size_t id_count = 1;
            bt_id_get(&ble_id, &id_count);
            // Reverse byte order so that it displays nicely through Reach
            for (int i = 0; i < sizeof(ble_id.a.val); i++)
                data->value.bytes_value.bytes[i] = ble_id.a.val[sizeof(ble_id.a.val) - i - 1];
            data->value.bytes_value.size = sizeof(ble_id.a.val);
            break;
        case PARAM_UPTIME:
            data->value.sint64_value = k_uptime_get();
            break;
        case PARAM_BUTTON_PRESSED:
            data->value.bool_value = main_get_button_pressed();
            break;
        case PARAM_IDENTIFY_LED_ON:
            data->value.bool_value = main_get_identify_led_on();
            break;
        case PARAM_RGB_LED_COLOR:
            data->value.enum_value = (uint32_t) main_get_rgb_led_state();
            break;
        case PARAM_RGB_LED_STATE:
            data->value.bitfield_value = (uint32_t) main_get_rgb_led_state();
            break;
        case PARAM_IDENTIFY:
            data->value.bool_value = main_identify_enabled();
            break;
        default:
            // Do nothing with the data, and assume that it is valid
            break;
    }
    return rval;
}

int app_handle_param_repo_write(cr_ParameterValue *data)
{
    int rval = 0;
    // If needed, check if data is valid before allowing the write to occur
    // This is only necessary if there are limits on the parameter outside of min/max values (for example, needing to be a multiple of 5)
    // switch (data->parameter_id)
    // {
    //     default:
    //         break;
    // }

#ifdef PARAM_REPO_USE_FILE_STORAGE
    // Only think about the NVM if file access hasn't failed
    if (!pr_file_access_failed)
    {
        // Check if the parameter is in our list of NVM parameters, these need to be stored properly
        for (int i = 0; i < nvm_param_count; i++)
        {
            if (data->parameter_id == nvm_param_ids[i])
            {
                I3_LOG(LOG_MASK_PARAMS, "Handling NVM write for parameter %u, NVM index %d", data->parameter_id, i);
                // Update entry
                rval = fs_open(&pr_file, PARAM_REPO_FILE, FS_O_RDWR);
                if (rval < 0)
                {
                    I3_LOG(LOG_MASK_ERROR, "Failed to open file to write parameter ID %u, error %d", data->parameter_id, rval);
                    pr_file_access_failed = true;
                    // No need to close the file since it hasn't been opened
                    break;
                }
                rval = fs_seek(&pr_file, sizeof(uint32_t) + (i * sizeof(cr_ParameterValue)), FS_SEEK_SET);
                if (rval < 0)
                {
                    I3_LOG(LOG_MASK_ERROR, "Failed to seek in file to write parameter ID %u, error %d", data->parameter_id, rval);
                }
                else
                {
                    rval = fs_write(&pr_file, data, sizeof(cr_ParameterValue));
                    if (rval < 0)
                    {
                        I3_LOG(LOG_MASK_ERROR, "Failed to write parameter ID %u, error %d", data->parameter_id, rval);
                    }
                    else
                    {
                        rval = 0;
                    }
                }
                if (!pr_file_access_failed)
                    fs_close(&pr_file);
                // Only one entry per parameter, no need to keep looking
                if (rval)
                    rval = cr_ErrorCodes_WRITE_FAILED;
                break;
            }
        }
    }
#endif // PARAM_REPO_USE_FILE_STORAGE

    switch (data->parameter_id)
    {
        case PARAM_USER_DEVICE_NAME:
            if (data->value.string_value[0] == 0)
                rnrfc_set_advertised_name(CONFIG_BT_DEVICE_NAME);
            else
                rnrfc_set_advertised_name(data->value.string_value);
            break;
        case PARAM_RGB_LED_STATE:
            main_set_rgb_led_state((uint8_t) data->value.bitfield_value);
            break;
        case PARAM_RGB_LED_COLOR:
            main_set_rgb_led_state((uint8_t) data->value.enum_value);
            break;
        case PARAM_IDENTIFY:
            main_enable_identify(data->value.bool_value);
            break;
        case PARAM_IDENTIFY_INTERVAL:
            main_set_identify_interval(data->value.float32_value);
            break;
        default:
            // Do nothing with the data, and assume that it is valid
            break;
    }
    return rval;
}

#ifdef INCLUDE_TIME_SERVICE
int crcb_time_get(cr_TimeGetResponse *response)
{
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    response->seconds_utc = (int64_t) now.tv_sec;
    response->has_timezone = sCr_param_val[PARAM_TIMEZONE_ENABLED].value.bool_value;
    if (!response->has_timezone)
    {
        response->seconds_utc += sCr_param_val[PARAM_TIMEZONE_OFFSET].value.sint32_value;
    }
    else
    {
        response->timezone = sCr_param_val[PARAM_TIMEZONE_OFFSET].value.sint32_value;
    }
    return 0;
}

int crcb_time_set(const cr_TimeSetRequest *request)
{
    struct timespec time = {.tv_sec = (time_t) request->seconds_utc};
    clock_settime(CLOCK_REALTIME, &time);
    if (request->has_timezone)
    {
        cr_ParameterValue param;
        param.parameter_id = PARAM_TIMEZONE_OFFSET;
        param.which_value = cr_ParameterValue_sint32_value_tag;
        param.timestamp = (uint32_t) k_uptime_get();
        param.value.sint32_value = request->timezone;
        crcb_parameter_write(PARAM_TIMEZONE_OFFSET, &param);
    }
    return 0;
}
#endif

#ifdef PARAM_REPO_USE_FILE_STORAGE
static uint32_t calculate_nvm_hash(void)
{
    uint32_t hash = 0;
    bool first_param_found = false;
    for (int i = 0; i < NUM_PARAMS; i++)
    {
        if (param_desc[i].storage_location != cr_StorageLocation_NONVOLATILE)
            continue;
        uint32_t *ptr = (uint32_t*) &param_desc[i];
        if (!first_param_found)
        {
            hash = ptr[0];
            first_param_found = true;
        }
        else
            hash ^= ptr[0];
        for (size_t i = 1; i < (sizeof(cr_ParameterInfo) / sizeof(uint32_t)); i++)
            hash ^= ptr[i];
    }
    return hash;
}
#endif // PARAM_REPO_USE_FILE_STORAGE