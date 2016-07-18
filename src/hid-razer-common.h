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

#ifndef HID_RAZER_COMMON_H
#define HID_RAZER_COMMON_H


//#################//
//### Constants ###//
//#################//

// The Razer vendor ID
#define USB_VENDOR_ID_RAZER 0x1532

// Each USB report has 90 bytes
#define RAZER_USB_REPORT_LEN 0x5A



//#############//
//### Types ###//
//#############//

union transaction_id_union {
    unsigned char id;
    struct transaction_parts {
        unsigned char device : 3;
        unsigned char id : 5;
    } parts;
};

union command_id_union {
    unsigned char id;
    struct command_id_parts {
        unsigned char direction : 1;
        unsigned char id : 7;
    } parts;
};

/* Status:
 * 0x00 New Command
 * 0x01 Command Busy
 * 0x02 Command Successful
 * 0x03 Command Failure
 * 0x04 Command No Response / Command Timeout
 * 0x05 Command Not Support
 *
 * Transaction ID used to group request-response, device useful when multiple devices are on one usb
 * Remaining Packets is the number of remaining packets in the sequence
 * Protocol Type is always 0x00
 * Data Size is the size of payload, cannot be greater than 80. 90 = header (8B) + data + CRC (1B) + Reserved (1B)
 * Command Class is the type of command being issued
 * Command ID is the type of command being send. Direction 0 is Host->Device, Direction 1 is Device->Host. AKA Get LED 0x80, Set LED 0x00
 *
 * */

struct razer_report {
    unsigned char           status;
    union                   transaction_id_union transaction_id;
    unsigned short          remaining_packets;  // Big Endian
    unsigned char           protocol_type;      // 0x0
    unsigned char           data_size;
    unsigned char           command_class;
    union command_id_union  command_id;
    unsigned char           arguments[80];
    unsigned char           crc;        // xor'ed bytes of report
    unsigned char           reserved;   // 0x0
};



//#################//
//### Functions ###//
//#################//

struct razer_report new_razer_report(unsigned char command_class,
    unsigned char command_id, unsigned char data_size);

int razer_send_control_msg(struct usb_device *usb_dev,void const *data,
    uint report_index, ulong wait_min, ulong wait_max);

int razer_get_usb_response(struct usb_device *usb_dev, uint report_index,
    struct razer_report* request_report,
    uint response_index, struct razer_report* response_report,
    ulong wait_min, ulong wait_max);

unsigned char razer_calculate_crc(struct razer_report *report);

void razer_print_erroneous_report(struct razer_report* report,
    char* driver_name, char* message);


#endif // HID_RAZER_COMMON_H
