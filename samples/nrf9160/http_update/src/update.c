/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
#include <zephyr.h>
#include <drivers/gpio.h>
#include <drivers/flash.h>
#include <nrf_modem.h>
#include <modem/lte_lc.h>
#include <modem/at_cmd.h>
#include <modem/at_notif.h>
#include <modem/libmodem.h>
#include <modem/modem_key_mgmt.h>
#include <net/fota_download.h>
#include <net/socket.h>
#include <nrf_socket.h>
#include <stdio.h>

#define LED_PORT DT_GPIO_LABEL(DT_ALIAS(led0), gpios)
#define TLS_SEC_TAG 42

static const struct device *gpiob;
static struct gpio_callback gpio_cb;
static struct k_work fota_work;

void (*finished_cb)(void);
const char *(*host_get)(void);
const char *(*file_get)(void);
bool (*two_leds)(void);

#ifndef CONFIG_USE_HTTPS
#define SEC_TAG (-1)
#else
#define SEC_TAG (TLS_SEC_TAG)
#endif

/**@brief Recoverable modem library error. */
void nrf_modem_recoverable_error_handler(uint32_t err)
{
	printk("Modem library recoverable error: %u\n", err);
}

int cert_provision(void)
{
	static const char cert[] = {
		#include "../cert/BaltimoreCyberTrustRoot"
	};
	BUILD_ASSERT(sizeof(cert) < KB(4), "Certificate too large");
	int err;
	bool exists;
	uint8_t unused;

	err = modem_key_mgmt_exists(TLS_SEC_TAG,
				    MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
				    &exists, &unused);
	if (err) {
		printk("Failed to check for certificates err %d\n", err);
		return err;
	}

	if (exists) {
		/* For the sake of simplicity we delete what is provisioned
		 * with our security tag and reprovision our certificate.
		 */
		err = modem_key_mgmt_delete(TLS_SEC_TAG,
					    MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN);
		if (err) {
			printk("Failed to delete existing certificate, err %d\n",
			       err);
		}
	}

	printk("Provisioning certificate\n");

	/*  Provision certificate to the modem */
	err = modem_key_mgmt_write(TLS_SEC_TAG,
				   MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
				   cert, sizeof(cert) - 1);
	if (err) {
		printk("Failed to provision certificate, err %d\n", err);
		return err;
	}

	return 0;
}

/**@brief Start transfer of the file. */
static void update_transfer_start(struct k_work *unused)
{
	int retval;
	char *apn = NULL;

	/* Functions for getting the host and file */
	retval = fota_download_start(host_get(),
				     file_get(),
				     SEC_TAG,
				     apn,
				     0);
	if (retval != 0) {
		/* Re-enable button callback */
		gpio_pin_interrupt_configure(gpiob,
					     DT_GPIO_PIN(DT_ALIAS(sw0), gpios),
					     GPIO_INT_EDGE_TO_ACTIVE);

		printk("fota_download_start() failed, err %d\n", retval);
		return;
	}

}

static int led_init(void)
{
	const struct device *dev;

	dev = device_get_binding(LED_PORT);
	if (dev == 0) {
		printk("Nordic nRF GPIO driver was not found!\n");
		return 1;
	}

	gpio_pin_configure(dev, DT_GPIO_PIN(DT_ALIAS(led0), gpios),
			   GPIO_OUTPUT_ACTIVE |
			   DT_GPIO_FLAGS(DT_ALIAS(led0), gpios));

	if (two_leds()) {
		gpio_pin_configure(dev, DT_GPIO_PIN(DT_ALIAS(led1), gpios),
				GPIO_OUTPUT_ACTIVE |
				DT_GPIO_FLAGS(DT_ALIAS(led1), gpios));
	}

	return 0;
}

void dfu_button_pressed(const struct device *gpiob, struct gpio_callback *cb,
			uint32_t pins)
{
	k_work_submit(&fota_work);
	gpio_pin_interrupt_configure(gpiob, DT_GPIO_PIN(DT_ALIAS(sw0), gpios),
				     GPIO_INT_DISABLE);
}


static int button_init(void)
{
	int err;

	gpiob = device_get_binding(DT_GPIO_LABEL(DT_ALIAS(sw0), gpios));
	if (gpiob == 0) {
		printk("Nordic nRF GPIO driver was not found!\n");
		return 1;
	}
	err = gpio_pin_configure(gpiob, DT_GPIO_PIN(DT_ALIAS(sw0), gpios),
				 GPIO_INPUT |
				 DT_GPIO_FLAGS(DT_ALIAS(sw0), gpios));
	if (err == 0) {
		gpio_init_callback(&gpio_cb, dfu_button_pressed,
			BIT(DT_GPIO_PIN(DT_ALIAS(sw0), gpios)));
		err = gpio_add_callback(gpiob, &gpio_cb);
	}
	if (err == 0) {
		err = gpio_pin_interrupt_configure(gpiob,
						   DT_GPIO_PIN(DT_ALIAS(sw0),
							       gpios),
						   GPIO_INT_EDGE_TO_ACTIVE);
	}
	if (err != 0) {
		printk("Unable to configure SW0 GPIO pin!\n");
		return 1;
	}
	return 0;
}

void fota_dl_handler(const struct fota_download_evt *evt)
{
	switch (evt->id) {
	case FOTA_DOWNLOAD_EVT_ERROR:
		printk("Received error from fota_download\n");
		/* Fallthrough */
	case FOTA_DOWNLOAD_EVT_FINISHED:
		/* Re-enable button callback */
		gpio_pin_interrupt_configure(gpiob,
					     DT_GPIO_PIN(DT_ALIAS(sw0), gpios),
					     GPIO_INT_EDGE_TO_ACTIVE);

		finished_cb();

		break;

	default:
		break;
	}
}

/**@brief Configures modem to provide LTE link.
 *
 * Blocks until link is successfully established.
 */
static void modem_configure(void)
{
#if defined(CONFIG_LTE_LINK_CONTROL)
	BUILD_ASSERT(!IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT),
			"This sample does not support auto init and connect");
	int err;
#if !defined(CONFIG_LIBMODEM_SYS_INIT)
	/* Initialize AT only if libmodem_init() is manually
	 * called by the main application
	 */
	err = at_notif_init();
	__ASSERT(err == 0, "AT Notify could not be initialized.");
	err = at_cmd_init();
	__ASSERT(err == 0, "AT CMD could not be established.");
#if defined(CONFIG_USE_HTTPS)
	err = cert_provision();
	__ASSERT(err == 0, "Could not provision root CA to %d", TLS_SEC_TAG);
#endif
#endif
	printk("LTE Link Connecting ...\n");
	err = lte_lc_init_and_connect();
	__ASSERT(err == 0, "LTE link could not be established.");
	printk("LTE Link Connected!\n");
#endif
}

int update_sample_init(void (*finished_cb_in)(void),
		       const char *(*host_get_in)(void),
		       const char *(file_get_in)(void),
		       bool (*two_leds_in)(void))
{
	int err;

	if (finished_cb_in == NULL || host_get_in == NULL || file_get_in == NULL
	    || two_leds_in ==NULL) {
		return -EINVAL;
	}

	finished_cb = finished_cb_in;
	host_get = host_get_in;
	file_get = file_get_in;
	two_leds = two_leds_in;

	k_work_init(&fota_work, update_transfer_start);

	modem_configure();

	err = button_init();
	if (err != 0) {
		return err;
	}

	err = led_init();
	if (err != 0) {
		return err;
	}

	err = fota_download_init(fota_dl_handler);
	if (err != 0) {
		return err;
	}

	return 0;
}
