/********************************************************************************************
 *
 * \date   2024
 *
 * \author i3 Product Development (JNP)
 *
 * \brief  Functions to handle executing Reach commands
 *
 ********************************************************************************************/

#include <zephyr/kernel.h>

#include "cr_stack.h"
#include "definitions.h"
#include "i3_log.h"

int crcb_command_execute(const uint8_t cid)
{
    int rval = 0;
    switch (cid)
    {
        case COMMAND_REBOOT:
            NVIC_SystemReset();
            break;
        case COMMAND_RESET_DEFAULTS:
            extern int param_repo_reset_nvm(void);
            rval = param_repo_reset_nvm();
            if (rval != 0)
            {
                extern void files_reset(void);
                files_reset();
            }
            break;
        case COMMAND_INVALIDATE_OTA_IMAGE:
            extern int ota_invalidate(void);
            rval = ota_invalidate();
            break;
        default:
            // Unsupported command
            return cr_ErrorCodes_INVALID_PARAMETER;
    }
    if (rval)
    {
        i3_log(LOG_MASK_ERROR, "Failed to execute command %u, error %d", cid, rval);
        rval = cr_ErrorCodes_NO_DATA;
    }
    return rval;
}