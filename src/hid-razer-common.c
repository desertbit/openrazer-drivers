/*
 * Razer Kernel Drivers
 * Copyright (c) 2016 Roland Singer <roland.singer@desertbit.com>
 * Based on Tim Theede <pez2001@voyagerproject.de> razer_chroma_drivers project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/printk.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/usb/input.h>
#include <linux/hid.h>

#include "hid-razer-common.h"

//###########################//
//### Version Information ###//
//###########################//

MODULE_AUTHOR("Roland Singer <roland.singer@desertbit.com>");
MODULE_DESCRIPTION("USB Razer common driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.0");



//##########################//
//### Exported Functions ###//
//##########################//

/*
 * Initialize a razer device struct.
 */
int razer_init_device(struct razer_device *razer_dev,
	struct usb_device *usb_dev)
{
	razer_dev->data     = NULL;
	razer_dev->usb_dev  = usb_dev;
	mutex_init(&razer_dev->lock);

	return 0;
}
EXPORT_SYMBOL_GPL(razer_init_device);



/*
 * Get an initialised razer report
 */
struct razer_report razer_new_report(void)
{
	struct razer_report new_report;

	memset(&new_report, 0, sizeof(struct razer_report));

	new_report.status             = RAZER_STATUS_NEW_COMMAND;
	new_report.transaction_id     = 0xFF;
	new_report.remaining_packets  = 0x00;
	new_report.protocol_type      = 0x00;
	new_report.reserved           = 0x00;
	new_report.command_class      = 0x00;
	new_report.command_id         = 0x00;
	new_report.data_size          = 0x00;

	return new_report;
}
EXPORT_SYMBOL_GPL(razer_new_report);



/*
 * Send an USB control report to the device.
 * Returns 0 on success.
 */
int _razer_send(struct razer_device *razer_dev, struct razer_report *report)
{
	const uint size = sizeof(struct razer_report);
	char *buf;
	int len;

	buf = kmemdup(report, size, GFP_KERNEL);
	if (buf == NULL)
		return -ENOMEM;

	len = usb_control_msg(razer_dev->usb_dev,
		usb_sndctrlpipe(razer_dev->usb_dev, 0),
		HID_REQ_SET_REPORT,                                 // Request
		USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_OUT, // Type
		0x300,                                              // Value
		razer_dev->report_index,                            // Index
		buf,                                                // Data
		size,                                               // Length
		USB_CTRL_SET_TIMEOUT);

	usleep_range(600, 800);

	kfree(buf);

	return ((len < 0) ? len : ((len != size) ? -EIO : 0));
}

int razer_send(struct razer_device *razer_dev, struct razer_report *report)
{
	int retval;

	mutex_lock(&razer_dev->lock);
	retval = _razer_send(razer_dev, report);
	mutex_unlock(&razer_dev->lock);

	return retval;
}
EXPORT_SYMBOL_GPL(razer_send);



/*
 * Get a response from the razer device.
 * Returns 0 on success.
 */
int _razer_receive(struct razer_device *razer_dev, struct razer_report *report)
{
	const uint size = sizeof(struct razer_report);
	int len;

	memset(report, 0, size);

	len = usb_control_msg(razer_dev->usb_dev,
		usb_rcvctrlpipe(razer_dev->usb_dev, 0),
		HID_REQ_GET_REPORT,                                   // Request
		USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_IN,    // Type
		0x300,                                                // Value
		razer_dev->report_index,                              // Index
		report,                                               // Data
		size,
		USB_CTRL_SET_TIMEOUT);

	usleep_range(600, 800);

	return ((len < 0) ? len : ((len != size) ? -EIO : 0));
}

int razer_receive(struct razer_device *razer_dev, struct razer_report *report)
{
	int retval;

	mutex_lock(&razer_dev->lock);
	retval = _razer_receive(razer_dev, report);
	mutex_unlock(&razer_dev->lock);

	return retval;
}
EXPORT_SYMBOL_GPL(razer_receive);



/*
 * Send a report and wait for a response.
 * Returns 0 on success.
 */
int _razer_send_with_response(struct razer_device *razer_dev,
	struct razer_report *request_r,
	struct razer_report *response_r)
{
	int retval, r;

	retval = _razer_send(razer_dev, request_r);
	if (retval != 0)
		return retval;

	// Retry 40 times when busy -> 125 milliseconds -> max 5 seconds wait
	for (r = 0; r < 40; r++) {
		retval = _razer_receive(razer_dev, response_r);
		if (retval != 0)
			return retval;

		if (response_r->command_class != request_r->command_class ||
			response_r->command_id != request_r->command_id) {
			dev_err(&razer_dev->usb_dev->dev,
					"razer_send_with_response: "
					"response commands do not match: "
					"Request Class: %d "
					"Request ID: %d "
					"Response Class: %d "
					"Response ID: %d\n",
					request_r->command_class,
					request_r->command_id,
					response_r->command_class,
					response_r->command_id);
			return -EINVAL;
		}

		switch (response_r->status) {
		case RAZER_STATUS_SUCCESS:
			return 0;

		case RAZER_STATUS_BUSY:
			msleep(125);
			continue;

		case RAZER_STATUS_FAILURE:
		case RAZER_STATUS_TIMEOUT:
		case RAZER_STATUS_NOT_SUPPORTED:
			return -EINVAL;

		default:
			dev_err(&razer_dev->usb_dev->dev,
					"razer_send_with_response: "
					"unknown response status 0x%x\n",
					response_r->status);
			return -EINVAL;
		}
	}

	dev_err(&razer_dev->usb_dev->dev, "razer_send_with_response: "
			"request failed: device is busy\n");

	return -EBUSY;
}

int razer_send_with_response(struct razer_device *razer_dev,
	struct razer_report *request_report,
	struct razer_report *response_report)
{
	int retval;

	mutex_lock(&razer_dev->lock);
	retval = _razer_send_with_response(razer_dev,
			request_report, response_report);
	mutex_unlock(&razer_dev->lock);

	return retval;
}
EXPORT_SYMBOL_GPL(razer_send_with_response);



/*
 * Send a report, wait for a response and check the response status.
 * Returns 0 on success.
 */
int razer_send_check_response(struct razer_device *razer_dev,
	struct razer_report *request_report)
{
	struct razer_report response_report;

	return razer_send_with_response(razer_dev,
			request_report, &response_report);
}
EXPORT_SYMBOL_GPL(razer_send_check_response);



/*
 * Calculate the checksum for the usb message
 *
 * Checksum byte is stored in the 2nd last byte in the messages payload.
 * The checksum is generated by XORing all the bytes in the report starting
 * at byte number 2 (0 based) and ending at byte 88.
 */
unsigned char razer_calculate_crc(struct razer_report *report)
{
	// Second to last byte of report is a simple checksum.
	// Just xor all bytes up with overflow and you are done.
	unsigned char crc = 0;
	unsigned char *_report = (unsigned char *)report;
	unsigned int i;

	for (i = 2; i < 88; i++)
		crc ^= _report[i];

	return crc;
}
EXPORT_SYMBOL_GPL(razer_calculate_crc);



/*
 * Detailed error print
 */
void razer_print_err_report(struct razer_report *report,
	char *driver_name, char *message)
{
	printk(KERN_WARNING "%s: %s. Status: %02x Transaction ID: %02x"
		" Data Size: %02x Command Class: %02x Command ID: %02x"
		" Params: %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
		driver_name,
		message,
		report->status,
		report->transaction_id,
		report->data_size,
		report->command_class,
		report->command_id,
		report->arguments[0], report->arguments[1],
		report->arguments[2], report->arguments[3],
		report->arguments[4], report->arguments[5],
		report->arguments[6], report->arguments[7],
		report->arguments[8], report->arguments[9]);
}
EXPORT_SYMBOL_GPL(razer_print_err_report);
