/********************************************************************************************
 *
 * \date   2024
 *
 * \author i3 Product Development (JNP)
 *
 * \brief  Setup and main UI code for the nRF Connect demo.  Based on the BLE throughput sample code.
 *
 ********************************************************************************************/

#include "main.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/posix/time.h>
#include <zephyr/storage/flash_map.h>

#include <dk_buttons_and_leds.h>

#include "i3_log.h"

#include "reach_nrf_connect.h"
#include "definitions.h"

static int littlefs_flash_erase(unsigned int id);
static int littlefs_mount(struct fs_mount_t *mp);

static void identify_task(void *arg, void *param2, void *param3);
static void set_identify(bool en);
static void button_handler_cb(uint32_t button_state, uint32_t has_changed);

#define PARTITION_NODE DT_NODELABEL(lfs1)

#if DT_NODE_EXISTS(PARTITION_NODE)
FS_FSTAB_DECLARE_ENTRY(PARTITION_NODE);
#else /* PARTITION_NODE */
FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(storage);
static struct fs_mount_t lfs_storage_mnt = {
	.type = FS_LITTLEFS,
	.fs_data = &storage,
	.storage_dev = (void *)FIXED_PARTITION_ID(storage_partition),
	.mnt_point = "/lfs",
};
#endif /* PARTITION_NODE */

	struct fs_mount_t *mountpoint =
#if DT_NODE_EXISTS(PARTITION_NODE)
		&FS_FSTAB_ENTRY(PARTITION_NODE)
#else
		&lfs_storage_mnt
#endif
		;

K_THREAD_STACK_DEFINE(identify_task_stack_area, 8192);
static struct k_thread identify_task_data;
static k_tid_t identify_task_id;


static struct button_handler button = {
	.cb = button_handler_cb,
};

static bool identify_enabled = false;
static uint16_t identify_interval_ms = 1000;
static bool identify_led_on = false;
static uint8_t rgb_led_state = RGB_LED_COLOR_OFF;
static bool button_pressed = false;

int main(void)
{
	boot_write_img_confirmed();

	littlefs_mount(mountpoint);

	dk_leds_init();
	dk_buttons_init(NULL);

    identify_task_id = k_thread_create(
        &identify_task_data, identify_task_stack_area,
        K_THREAD_STACK_SIZEOF(identify_task_stack_area),
        identify_task,
        NULL, NULL, NULL,
        2,
        K_FP_REGS,
        K_NO_WAIT);

	dk_button_handler_add(&button);
	rnrfc_init();

	extern void files_init(void);
	files_init();

	extern void cli_init(void);
	cli_init();

	// Advertise the user device name if it's been set
	if (sCr_param_val[PARAM_USER_DEVICE_NAME].value.string_value[0] != 0)
		rnrfc_set_advertised_name(sCr_param_val[PARAM_USER_DEVICE_NAME].value.string_value);

	main_set_rgb_led_state(RGB_LED_COLOR_GREEN);

	struct timespec nts;
    nts.tv_sec = (time_t) 0;
    nts.tv_nsec = 0;
    clock_settime(CLOCK_REALTIME, &nts);

	return 0;
}

void main_enable_identify(bool en)
{
	identify_enabled = en;
	k_wakeup(identify_task_id);
}

bool main_identify_enabled(void)
{
	return identify_enabled;
}

void main_set_identify_interval(float seconds)
{
	if (seconds < 0)
		return;
	identify_interval_ms = (uint16_t) ((seconds * 1000) / 2);
	k_wakeup(identify_task_id);
}

bool main_get_identify_led_on(void)
{
	return identify_led_on;
}

uint8_t main_get_rgb_led_state(void)
{
	return rgb_led_state;
}

void main_set_rgb_led_state(uint8_t state)
{
	rgb_led_state = state;
	dk_set_led(1, (state & RGB_LED_STATE_BIT_RED) ? 1:0);
	dk_set_led(2, (state & RGB_LED_STATE_BIT_GREEN) ? 1:0);
	dk_set_led(3, (state & RGB_LED_STATE_BIT_BLUE) ? 1:0);
}

bool main_get_button_pressed(void)
{
	return button_pressed;
}

static int littlefs_flash_erase(unsigned int id)
{
	const struct flash_area *pfa;
	int rc;

	rc = flash_area_open(id, &pfa);
	if (rc < 0) {
		I3_LOG(LOG_MASK_ERROR, "Unable to find flash area %u: %d",
			id, rc);
		return rc;
	}

	I3_LOG(LOG_MASK_ALWAYS, "Area %u at 0x%x on %s for %u bytes",
		   id, (unsigned int)pfa->fa_off, pfa->fa_dev->name,
		   (unsigned int)pfa->fa_size);

	/* Optional wipe flash contents */
	if (IS_ENABLED(CONFIG_APP_WIPE_STORAGE)) {
		rc = flash_area_erase(pfa, 0, pfa->fa_size);
		I3_LOG(LOG_MASK_ERROR, "Erasing flash area ... %d", rc);
	}

	flash_area_close(pfa);
	return rc;
}

static int littlefs_mount(struct fs_mount_t *mp)
{
	int rc;

	rc = littlefs_flash_erase((uintptr_t)mp->storage_dev);
	if (rc < 0) {
		return rc;
	}

	/* Do not mount if auto-mount has been enabled */
#if !DT_NODE_EXISTS(PARTITION_NODE) ||						\
	!(FSTAB_ENTRY_DT_MOUNT_FLAGS(PARTITION_NODE) & FS_MOUNT_FLAG_AUTOMOUNT)
	rc = fs_mount(mp);
	if (rc < 0) {
		I3_LOG(LOG_MASK_ERROR, "Failed to mount ID %" PRIuPTR " at %s, error %d",
		       (uintptr_t)mp->storage_dev, mp->mnt_point, rc);
		return rc;
	}
	I3_LOG(LOG_MASK_ALWAYS, "%s mounted: %d", mp->mnt_point, rc);
#else
	I3_LOG(LOG_MASK_ALWAYS, "%s automounted", mp->mnt_point);
#endif
	return 0;
}

void rnrfc_app_handle_ble_connection(void)
{
	main_set_rgb_led_state(RGB_LED_COLOR_BLUE);
    return;
}

void rnrfc_app_handle_ble_disconnection(void)
{
	main_set_rgb_led_state(RGB_LED_COLOR_GREEN);
    return;
}

static void identify_task(void *arg, void *param2, void *param3)
{
	while (1)
	{
		if (identify_enabled)
		{
			set_identify(!identify_led_on);
			k_msleep(identify_interval_ms);
		}
		else
		{
			set_identify(false);
			// Sleep until woken up by a state change
			k_sleep(K_FOREVER);
		}
	}
}

static void set_identify(bool en)
{
	identify_led_on = en;
	dk_set_led(0, en ? 1:0);
}

static void button_handler_cb(uint32_t button_state, uint32_t has_changed)
{
	ARG_UNUSED(has_changed);
	button_pressed = button_state & DK_BTN1_MSK;
	if (button_pressed)
		main_enable_identify(!identify_enabled);
}