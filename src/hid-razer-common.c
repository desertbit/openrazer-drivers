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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

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
 * Get an initialised razer report
 */
struct razer_report new_razer_report(unsigned char command_class, unsigned char command_id, unsigned char data_size)
{
    struct razer_report new_report;
    memset(&new_report, 0, sizeof(struct razer_report));

    new_report.status               = RAZER_STATUS_NEW_COMMAND;
    new_report.transaction_id.id    = 0xFF;
    new_report.remaining_packets    = 0x00;
    new_report.protocol_type        = 0x00;
    new_report.command_class        = command_class;
    new_report.command_id.id        = command_id;
    new_report.data_size            = data_size;

    return new_report;
}

EXPORT_SYMBOL_GPL(new_razer_report);



/*
 * Send USB control report to the keyboard
 * Usually index = 0x02
 * Returns 0 on success.
 */
int razer_send_control_msg(struct usb_device *usb_dev, void const *data,
	uint report_index, ulong wait_min, ulong wait_max)
{
	uint request = HID_REQ_SET_REPORT; // 0x09
	uint request_type = USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_OUT; // 0x21
    uint value = 0x300;
    uint size = RAZER_USB_REPORT_LEN;
    char *buf;
    int len;

    buf = kmemdup(data, size, GFP_KERNEL);
    if (buf == NULL) {
        return -ENOMEM;
    }

    // Send usb control message
    len = usb_control_msg(usb_dev, usb_sndctrlpipe(usb_dev, 0),
            request,      // Request
            request_type, // RequestType
            value,        // Value
            report_index, // Index
            buf,          // Data
            size,         // Length
            USB_CTRL_SET_TIMEOUT);

    // Wait
    usleep_range(wait_min, wait_max);

    // Free the memory again.
    kfree(buf);

    if (len != size) {
        printk(KERN_WARNING "razer device: device data transfer failed");
        return -EIO;
    }

    return 0;
}

EXPORT_SYMBOL_GPL(razer_send_control_msg);



/*
 * Get a response from the razer device
 *
 * Makes a request like normal, this must change a variable in the device as then we
 * tell it give us data and it gives us a report.
 *
 * Request report is the report sent to the device specifing what response we want
 * Response report will get populated with a response
 *
 * Returns 0 on success.
 */
int razer_get_usb_response(struct usb_device *usb_dev, uint report_index,
	struct razer_report* request_report,
	uint response_index, struct razer_report* response_report,
	ulong wait_min, ulong wait_max)
{
    uint request = HID_REQ_GET_REPORT; // 0x01
    uint request_type = USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_DIR_IN; // 0xA1
    uint value = 0x300;

    uint size = RAZER_USB_REPORT_LEN;
    int len;
    int retval;

    memset(response_report, 0, sizeof(struct razer_report));

    // Send the request to the device.
    retval = razer_send_control_msg(usb_dev, request_report, report_index, wait_min, wait_max);
	if (retval != 0) {
		printk(KERN_WARNING "razer device: invalid USB repsonse: request failed: %d\n", retval);
		return retval;
	}

    // Now ask for reponse
    len = usb_control_msg(usb_dev, usb_rcvctrlpipe(usb_dev, 0),
          request,         // Request
          request_type,    // RequestType
          value,           // Value
          response_index,  // Index
          response_report, // Data
          size,
          USB_CTRL_SET_TIMEOUT);

    usleep_range(wait_min, wait_max);

    // Error if report is wrong length
    if (len != size) {
        printk(KERN_WARNING "razer device: invalid USB repsonse: USB report length: %d\n", len);
        return -EIO;
    }

    return 0;
}

EXPORT_SYMBOL_GPL(razer_get_usb_response);



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
    unsigned char *_report = (unsigned char*)report;

    unsigned int i;
    for(i = 2; i < 88; i++) {
        crc ^= _report[i];
    }

    return crc;
}

EXPORT_SYMBOL_GPL(razer_calculate_crc);



/*
 * Detailed error print
 */
void razer_print_err_report(struct razer_report* report, char* driver_name, char* message)
{
    printk(KERN_WARNING "%s: %s. Status: %02x Transaction ID: %02x Data Size: %02x Command Class: %02x Command ID: %02x Params: %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
        driver_name,
        message,
        report->status,
        report->transaction_id.id,
        report->data_size,
        report->command_class,
        report->command_id.id,
        report->arguments[0], report->arguments[1], report->arguments[2],
        report->arguments[3], report->arguments[4], report->arguments[5],
        report->arguments[6], report->arguments[7], report->arguments[8],
        report->arguments[9]);
}

EXPORT_SYMBOL_GPL(razer_print_err_report);
