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

#ifndef __HID_RAZER_COMMON_H
#define __HID_RAZER_COMMON_H

#include <linux/usb.h>
#include <linux/types.h>


//#################//
//### Constants ###//
//#################//

// The Razer vendor ID
// TODO: move to hid-ids.h on linux mainline inclusion.
#define USB_VENDOR_ID_RAZER                     0x1532
#define USB_DEVICE_ID_RAZER_BLADE_STEALTH_2016  0x0205
#define USB_DEVICE_ID_RAZER_BLADE_14_2016       0x020F
#define USB_DEVICE_ID_RAZER_BLACKWIDOW_CHROMA   0x0203



//#############//
//### Types ###//
//#############//

enum razer_status {
    RAZER_STATUS_NEW_COMMAND    = 0x00,
    RAZER_STATUS_BUSY           = 0x01,
    RAZER_STATUS_SUCCESS        = 0x02,
    RAZER_STATUS_FAILURE        = 0x03,
    RAZER_STATUS_TIMEOUT        = 0x04,
    RAZER_STATUS_NOT_SUPPORTED  = 0x05
};

struct razer_device {
    struct usb_device   *usb_dev;
    struct mutex        lock;
    uint                report_index;   // The report index to use.
};

struct razer_rgb {
    unsigned char r, g, b;
};

struct razer_report {
    unsigned char   status;
    unsigned char   transaction_id;     // Used to group request-response.
    unsigned short  remaining_packets;  // Number of remaining packets in the sequence (Big Endian)
    unsigned char   protocol_type;      // Protocol Type is always 0x0.
    unsigned char   data_size;          // Size of payload, cannot be greater than 80. 90 = header (8B) + data + CRC (1B) + Reserved (1B)
    unsigned char   command_class;      // Type of command being send. Direction 0 is Host->Device, Direction 1 is Device->Host. AKA Get LED 0x80, Set LED 0x00
    unsigned char   command_id;         // Type of command being issued.
    unsigned char   arguments[80];
    unsigned char   crc;                // xor'ed bytes of report
    unsigned char   reserved;           // Is always 0x0.
};



//#################//
//### Functions ###//
//#################//

int razer_init_device(struct razer_device *razer_dev,
    struct usb_device *usb_dev);

struct razer_report razer_new_report(unsigned char command_class,
    unsigned char command_id, unsigned char data_size);

int razer_send(struct razer_device *razer_dev, struct razer_report* report);

int razer_receive(struct razer_device *razer_dev, struct razer_report* report);

int razer_send_with_response(struct razer_device *razer_dev,
    struct razer_report* request_report, struct razer_report* response_report);

int razer_send_check_response(struct razer_device *razer_dev,
    struct razer_report* request_report);

unsigned char razer_calculate_crc(struct razer_report *report);

void razer_print_err_report(struct razer_report* report,
    char* driver_name, char* message);


#endif // __HID_RAZER_COMMON_H
