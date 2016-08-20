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

#ifndef __HID_RAZER_COMMON_H
#define __HID_RAZER_COMMON_H

#include <linux/usb.h>
#include <linux/types.h>

//#############//
//### Types ###//
//#############//

enum razer_status {
	RAZER_STATUS_NEW_COMMAND   = 0x00,
	RAZER_STATUS_BUSY          = 0x01,
	RAZER_STATUS_SUCCESS       = 0x02,
	RAZER_STATUS_FAILURE       = 0x03,
	RAZER_STATUS_TIMEOUT       = 0x04,
	RAZER_STATUS_NOT_SUPPORTED = 0x05
};

struct razer_device {
	struct usb_device *usb_dev;
	struct mutex      lock;
	uint              report_index;  // The report index to use.
	void              *data;         // Optional custom data.
};

struct razer_rgb {
	unsigned char r, g, b;
};

// Report send or received from the device.
// transaction_id: Used to group request-response.
// remaining_packets: Number of remaining packets in the sequence (Big Endian).
// protocol_type: Always 0x0.
// data_size:     Size of payload, cannot be greater than 80.
//                90 = header (8B) + data + CRC (1B) + Reserved (1B)
// command_id:    Type of command being issued.
// command_class: Type of command being send. Direction 0 is Host->Device.
//                Direction 1 is Device->Host. AKA Get LED 0x80, Set LED 0x00
// crc:           xor'ed bytes of report
// reserved:      Is always 0x0.
struct razer_report {
	unsigned char   status;
	unsigned char   transaction_id;
	unsigned short  remaining_packets;
	unsigned char   protocol_type;
	unsigned char   data_size;
	unsigned char   command_class;
	unsigned char   command_id;
	unsigned char   arguments[80];
	unsigned char   crc;
	unsigned char   reserved;
};

//#################//
//### Functions ###//
//#################//

int razer_init_device(struct razer_device *razer_dev,
		      struct usb_device *usb_dev);

struct razer_report razer_new_report(void);

int razer_send(struct razer_device *razer_dev, struct razer_report *report);

int razer_receive(struct razer_device *razer_dev, struct razer_report *report);

int razer_send_with_response(struct razer_device *razer_dev,
			     struct razer_report *request_report,
			     struct razer_report *response_report);

int razer_send_check_response(struct razer_device *razer_dev,
			      struct razer_report *request_report);

unsigned char razer_calculate_crc(struct razer_report *report);

void razer_print_err_report(struct razer_report *report,
			    char *driver_name, char *message);

#endif // __HID_RAZER_COMMON_H
