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

typedef enum
{
  SEQUENCE_AUTOBIOGRAPHY,
  SEQUENCE_REDESIGN_YOUR_LOGO,
  SEQUENCE_SAY_HELLO,
  SEQUENCE_INACTIVE = 0xFF
} sequence_t;

static uint32_t times_clicked = 0;
static uint8_t sequence_position = 0;
static sequence_t active_sequence = SEQUENCE_INACTIVE;

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
        case COMMAND_CLICK_FOR_WISDOM:
        {
            I3_LOG(LOG_MASK_ALWAYS, TEXT_BOLDMAGENTA "Dispensing wisdom*...");
            I3_LOG(LOG_MASK_ALWAYS, TEXT_BOLDMAGENTA "* wisdom sometimes comes from unexpected places.  like maybe an error report?");
            if (times_clicked > 3)
            {
                int64_t time = k_uptime_get() / 100;
                if (active_sequence == SEQUENCE_INACTIVE)
                {
                    // Activate an easter egg based on the uptime
                    if (time % 10 == SEQUENCE_AUTOBIOGRAPHY)
                        active_sequence = SEQUENCE_AUTOBIOGRAPHY;
                    else if (time % 10 == SEQUENCE_REDESIGN_YOUR_LOGO)
                        active_sequence = SEQUENCE_REDESIGN_YOUR_LOGO;
                    else if (time % 10 == SEQUENCE_SAY_HELLO)
                        active_sequence = SEQUENCE_SAY_HELLO;
                }
                if (active_sequence != SEQUENCE_INACTIVE)
                {
                    // An easter egg has already been activated, continue that sequence until it is exhausted
                    switch (active_sequence)
                    {
                        case SEQUENCE_AUTOBIOGRAPHY:
                        {
                            // A comment made during David Bowie's 50th birthday live chat on AOL (Source: https://www.bowiewonderworld.com/chats/dbchat0197.htm)
                            switch (sequence_position)
                            {
                            case 0:
                                cr_report_error(1997, "I'm looking for backing for an unauthorized auto-biography that I am writing.");
                                break;
                            default:
                                cr_report_error(1997, "Hopefully, this will sell in such huge numbers that I will be able to sue myself for an extraordinary amount of money "
                                                      "and finance the film version in which I will play everybody.");
                                sequence_position = 0;
                                active_sequence = SEQUENCE_INACTIVE;
                                break;
                            }
                            break;
                        }
                        case SEQUENCE_REDESIGN_YOUR_LOGO:
                        {
                            // Lyrics from "Redesign Your Logo" by Lemon Demon
                            switch (sequence_position)
                            {
                            case 0:
                                cr_report_error(2016, "Redesign your logo, we know what we're doing.  We are here to help you; everything's connected");
                                break;
                            case 1:
                                cr_report_error(2016, "Time is of the essence, we live in the future.  Color makes us hungry; everything's connected");
                                break;
                            case 2:
                                cr_report_error(2016, "Redesign your logo, we know how to do it.  Make the calculations, put them into action");
                                break;
                            default:
                                cr_report_error(2016, "We will find the angle, starting with convention.  On to innovation; everything's connected");
                                sequence_position = 0;
                                active_sequence = SEQUENCE_INACTIVE;
                                break;
                            }
                            break;
                        }
                        case SEQUENCE_SAY_HELLO:
                        {
                            // Lyrics from "Say Hello" by Laurie Anderson
                            switch (sequence_position)
                            {
                            case 0:
                                cr_report_error(1984, "A certain American religious sect has been looking at conditions of the world during the Flood");
                                break;
                            case 1:
                                cr_report_error(1984,
                                                "According to their calculations, during the Flood the winds, tides and currents were in an overall southeasterly direction");
                                break;
                            case 2:
                                cr_report_error(1984,
                                                "This would mean that in order for Noah's Ark to have ended up on Mount Ararat, it would have to have started out several thousand miles to the west");
                                break;
                            case 3:
                                cr_report_error(1984,
                                                "This would then locate pre-Flood civilization in the area of Upstate New York, and the Garden of Eden roughly in New York City");
                                break;
                            case 4:
                                cr_report_error(1984, "Now, in order to get from one place to another, something must move");
                                break;
                            case 5:
                                cr_report_error(1984, "No one in New York remembers moving, and there are no traces of Biblical history in the Upstate New York area");
                                break;
                            default:
                                cr_report_error(1984, "So we are led to the only available conclusion in this time warp, and that is that the Ark has simply not left yet");
                                sequence_position = 0;
                                active_sequence = SEQUENCE_INACTIVE;
                                break;
                            }
                            break;
                        }
                        default:
                            break;
                    }
                    if (active_sequence != SEQUENCE_INACTIVE)
                        sequence_position++;
                    break;
                }
            }
            switch (times_clicked % 4)
            {
            case 0:
                cr_report_error(cr_ErrorCodes_NO_ERROR,
                                "The message reported by cr_report_error() can be up to 180 characters long.  An error code of cr_ErrorCodes_NO_ERROR (%d) is displayed differently",
                                cr_ErrorCodes_NO_ERROR);
                break;
            case 1:
                cr_report_error(cr_ErrorCodes_NO_DATA, "Informative error reporting code can often make it easy to locate the source of an error.  "
                                                       "Large and layered systems often have no way to report errors up the chain.");
                break;
            case 2:
                cr_report_error(9999, "Reach offers a way to communicate errors from the device to the developer.  "
                                      "Calls to cr_report_errror() result in messages like this.  Error codes can be any integer value.");
                break;
            case 3:
                cr_report_error(3, "Is that all there is?");
                break;
            }
            times_clicked++;
            break;
        }
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