/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <stdint.h>
#include <zephyr.h>
#include <flash.h>
#include <bsd.h>
#include <nrf_socket.h>
#include <download_client.h>
#include <logging/log.h>
#include <gpio.h>

#define LED_PORT	LED0_GPIO_CONTROLLER

static struct	device	*flash_dev;
static bool	is_flash_page_erased[FLASH_PAGE_MAX_CNT];
static bool	start_dfu;
static struct	gpio_callback gpio_cb;
static u32_t	flash_address;

static int app_download_client_event_handler(struct download_client *const dfu,
			enum download_client_evt event, u32_t status);


static struct download_client dfu = {
	.host = CONFIG_DOWNLOAD_HOST,
	.resource = CONFIG_DOWNLOAD_FILE,
	.callback = app_download_client_event_handler
};


/**@brief Recoverable BSD library error. */
void bsd_recoverable_error_handler(uint32_t err)
{
	printk("bsdlib recoverable error: %lu\n", err);
}

/**@brief Irrecoverable BSD library error. */
void bsd_irrecoverable_error_handler(uint32_t err)
{
	printk("bsdlib irrecoverable error: %lu\n", err);
}


/**@brief Initialize application. */
static void app_dfu_init(void)
{
	int i;

	flash_address = CONFIG_FLASH_WRITE_OFFSET;
	for (i = 0; i < FLASH_PAGE_MAX_CNT; i++) {
		is_flash_page_erased[i] = false;
	}
	flash_dev = device_get_binding(DT_FLASH_DEV_NAME);
	if (flash_dev == 0) {
		printk("Nordic nRF flash driver was not found!\n");
		return;
	}

	int retval = download_client_init(&dfu);

	if (retval != 0) {
		printk("download_client_init() failed, err %d", retval);
		return;
	}
}


/**@brief Start transfer of the file. */
static void app_dfu_transfer_start(void)
{
	int retval = download_client_connect(&dfu);

	if (retval != 0) {
		printk("download_client_connect() failed, err %d",
			retval);
		return;
	}

	retval = download_client_start(&dfu);
	if (retval != 0) {
		printk("download_client_start() failed, err %d",
			retval);
		return;
	}
}


static int app_download_client_event_handler(
	struct download_client *const dfu,
	enum download_client_evt event, u32_t error)
{
	int err;

	switch (event) {
	case DOWNLOAD_CLIENT_EVT_DOWNLOAD_FRAG: {
		struct flash_pages_info info;

		err = flash_get_page_info_by_offs(flash_dev, flash_address,
			&info);
		if (err != 0) {
			printk("flash_get_page_info_by_offs returned error %d\n",
				err);
			return 1;
		}
		flash_write_protection_set(flash_dev, false);
		if (!is_flash_page_erased[info.index]) {
			err = flash_erase(flash_dev, flash_address, info.size);
			if (err != 0) {
				printk("flash_erase returned error %d at address %08x\n",
					err, flash_address);
				return 1;
			}
			is_flash_page_erased[info.index] = true;
		}
		err = flash_write(flash_dev, flash_address, dfu->fragment,
							dfu->fragment_size);
		flash_write_protection_set(flash_dev, true);
		if (err != 0) {
			printk("Flash write error %d at address %08x\n",
				err, flash_address);
			return 1;
		}
		flash_address += dfu->fragment_size;
		break;
	}

	case DOWNLOAD_CLIENT_EVT_DOWNLOAD_DONE:
		printk("DOWNLOAD_CLIENT_EVT_DOWNLOAD_DONE");
		break;

	case DOWNLOAD_CLIENT_EVT_ERROR: {
		download_client_disconnect(dfu);
		printk("Download client error, please restart "
				"the application\n");
		break;
	}
	default:
		break;
	}
	return 0;
}


/**@brief Turn on LED0 and LED1 if CONFIG_APPLICATION_VERSION
 * is 2 and LED0 otherwise.
 */
static void led_app_version(void)
{
	struct device *dev;

	dev = device_get_binding(LED_PORT);
	if (dev == 0) {
		printk("Nordic nRF GPIO driver was not found!\n");
		return;
	}

	gpio_pin_configure(dev, LED0_GPIO_PIN, GPIO_DIR_OUT);
	gpio_pin_write(dev, LED0_GPIO_PIN, 1);

#if CONFIG_APPLICATION_VERSION == 2
	gpio_pin_configure(dev, LED1_GPIO_PIN, GPIO_DIR_OUT);
	gpio_pin_write(dev, LED1_GPIO_PIN, 1);
#endif
}

void dfu_button_pressed(struct device *gpiob, struct gpio_callback *cb,
		    u32_t pins)
{
	start_dfu = true;
}

static void dfu_button_init(void)
{
	int err;
	struct device *gpiob;

	gpiob = device_get_binding(SW0_GPIO_CONTROLLER);
	if (gpiob == 0) {
		printk("Nordic nRF GPIO driver was not found!\n");
		return;
	}
	err = gpio_pin_configure(gpiob, SW0_GPIO_PIN,
		GPIO_DIR_IN		|
		GPIO_INT		|
		GPIO_PUD_PULL_UP|
		GPIO_INT_EDGE	|
		GPIO_INT_ACTIVE_LOW);
	if (err == 0) {
		gpio_init_callback(&gpio_cb, dfu_button_pressed,
			BIT(SW0_GPIO_PIN));
		err = gpio_add_callback(gpiob, &gpio_cb);
	}
	if (err == 0) {
		err = gpio_pin_enable_callback(gpiob, SW0_GPIO_PIN);
	}
	if (err != 0) {
		printk("Unable to configure SW0_GPIO_PIN!\n");
	}
}

static void application_update(void)
{
	start_dfu = false;
	app_dfu_init();
	app_dfu_transfer_start();

	while (true) {
		if ((dfu.status == DOWNLOAD_CLIENT_STATUS_HALTED) ||
			(dfu.status == DOWNLOAD_CLIENT_ERROR)) {
			printk("Something went wrong, please restart the application\n");
			break;
		} else if (dfu.status ==
			DOWNLOAD_CLIENT_STATUS_DOWNLOAD_COMPLETE) {
			printk("Download complete\n");
			break;
		}
		download_client_process(&dfu);
	}
	download_client_disconnect(&dfu);
}


int main(void)
{
	boot_write_img_confirmed();
	start_dfu = false;
	dfu_button_init();
	led_app_version();
	while (true) {
		if (start_dfu) {
			application_update();
		}
	}

	return 0;
}
