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
#include <linux/dmi.h>

#include "hid-razer-common.h"



//###########################//
//### Version Information ###//
//###########################//

#define DRIVER_AUTHOR "Roland Singer <roland.singer@desertbit.com>"
#define DRIVER_DESC "USB HID Razer Keyboard"
#define DRIVER_LICENSE "GPL v2"

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE(DRIVER_LICENSE);



//#################//
//### Constants ###//
//#################//

#define RAZER_KBD_WAIT_MIN_US 600
#define RAZER_KBD_WAIT_MAX_US 800

// Device IDs
#define USB_DEVICE_ID_RAZER_BLADE_STEALTH_2016 0x0205
#define USB_DEVICE_ID_RAZER_BLADE_14_2016 0x020F



//#############//
//### Types ###//
//#############//

struct razer_kbd_device {
    struct usb_device *usbdev;
    struct hid_device *hiddev;
};



//########################//
//### Helper functions ###//
//########################//

// Send a report to the keyboard.
int razer_set_report(struct usb_device *usb_dev,void const *data)
{
    return razer_send_control_msg(usb_dev, data, 0x02, RAZER_KBD_WAIT_MIN_US, RAZER_KBD_WAIT_MAX_US);
}

// Send a report to the keyboard and obtain a response report.
int razer_get_report(struct usb_device *usb_dev,
    struct razer_report *request_report,
    struct razer_report *response_report)
{
    return razer_get_usb_response(usb_dev, 0x02, request_report, 0x02,
        response_report, RAZER_KBD_WAIT_MIN_US, RAZER_KBD_WAIT_MAX_US);
}

// Reset the keyboard.
int razer_reset(struct usb_device *usb_dev)
{
    int retval;
    struct razer_report report = new_razer_report(0x03, 0x00, 0x05);
    report.arguments[0] = 0x01; // LED Class
    report.arguments[1] = 0x08; // LED ID Game mode
    report.arguments[2] = 0x00; // Off
    report.crc = razer_calculate_crc(&report);
    retval = razer_set_report(usb_dev, &report);
    return retval;
}

// Get the firmware version.
int razer_get_firmware_version(struct usb_device *usb_dev, unsigned char* fw_string)
{
    int retval;
    struct razer_report response_report;
    struct razer_report request_report = new_razer_report(0x00, 0x81, 0x02);
    request_report.crc = razer_calculate_crc(&request_report);

    retval = razer_get_report(usb_dev, &request_report, &response_report);
    if(retval != 0)
    {
        razer_print_erroneous_report(&response_report, "hid-razer-kbd", "Invalid Report Length");
        return -1;
    }

    if(response_report.status == 0x02 &&
        response_report.command_class == 0x00 &&
        response_report.command_id.id == 0x81)
    {
        sprintf(fw_string, "v%d.%d", response_report.arguments[0], response_report.arguments[1]);
        retval = response_report.arguments[2];
    }
    else {
        razer_print_erroneous_report(&response_report, "hid-razer-kbd", "Invalid Report Type");
        return -1;
    }

    return retval;
}

// Get brightness of the keyboard.
int razer_get_brightness(struct usb_device *usb_dev)
{
    int retval;
    struct razer_report response_report;

    // Class LED Lighting, Command Set state, 3 Bytes of parameters
    struct razer_report request_report = new_razer_report(0x03, 0x83, 0x03);
    request_report.arguments[0] = 0x01;
    request_report.data_size = 0x02;
    request_report.command_class = 0x0E;
    request_report.command_id.id = 0x84;

    // Calculate the checksum
    request_report.crc = razer_calculate_crc(&request_report);

    retval = razer_get_report(usb_dev, &request_report, &response_report);
    if(retval != 0) {
        razer_print_erroneous_report(&response_report, "hid-razer-kbd", "Invalid Report Length");
        return -1;
    }

    // For the Razer Blades
    if(response_report.status == 0x02 &&
        response_report.command_class == 0x0E &&
        response_report.command_id.id == 0x84)
    {
        retval = response_report.arguments[1];
    }
    else {
        razer_print_erroneous_report(&response_report, "hid-razer-kbd", "Invalid Report Type");
        return -1;
    }

    return retval;
}

// Set the keyboard brightness.
int razer_set_brightness(struct usb_device *usb_dev, unsigned char brightness)
{
    int retval;

    struct razer_report report = new_razer_report(0x03, 0x03, 0x03);
    report.arguments[0] = 0x01;
    report.data_size = 0x02;
    report.command_class = 0x0E;
    report.command_id.id = 0x04;
    report.arguments[1] = brightness;

    // Calculate the checksum
    report.crc = razer_calculate_crc(&report);

    retval = razer_set_report(usb_dev, &report);
    if(retval != 0) {
        razer_print_erroneous_report(&report, "hid-razer-kbd", "request failed");
        return -1;
    }

    return 0;
}

// Set the logo lighting state (on/off only)
int razer_set_logo(struct usb_device *usb_dev, unsigned char state)
{
    int retval;
    struct razer_report report = new_razer_report(0x03, 0x00, 0x03);

    if (state != 0 && state != 1) {
        printk(KERN_WARNING "hid-razer-kbd: logo lighting state must be either 0 or 1. Got: %d", state);
        return -1;
    }

    report.arguments[0] = 0x01; // LED Class
    report.arguments[1] = 0x04; // LED ID, Logo
    report.arguments[2] = state; // State
    report.crc = razer_calculate_crc(&report);

    retval = razer_set_report(usb_dev, &report);
    if(retval != 0) {
        razer_print_erroneous_report(&report, "hid-razer-kbd", "request failed");
        return -1;
    }

    return 0;
}

// Toggle FN key
int razer_set_fn_toggle(struct usb_device *usb_dev, unsigned char state)
{
    int retval;
    struct razer_report report = new_razer_report(0x02, 0x06, 0x02);

    if (state != 0 && state != 1) {
        printk(KERN_WARNING "hid-razer-kbd: Toggle FN, state must be either 0 or 1. Got: %d", state);
        return -1;
    }

    report.arguments[0] = 0x00;
    report.arguments[1] = state; // State
    report.crc = razer_calculate_crc(&report);

    retval = razer_set_report(usb_dev, &report);
    if(retval != 0) {
        razer_print_erroneous_report(&report, "hid-razer-kbd", "request failed");
        return -1;
    }

    return 0;
}



//#########################//
//### Device Attributes ###//
//#########################//

/*
 * Read device file "get_serial"
 * Gets the serial number from the device,
 * Returns a string.
 */
static ssize_t razer_attr_read_get_serial(struct device *dev,
    struct device_attribute *attr, char *buf)
{
    char serial_string[100] = "";

    // Stealth and the Blade does not have serial via USB, so get it from the DMI table.
    strcpy(serial_string, dmi_get_system_info(DMI_PRODUCT_SERIAL));
    return sprintf(buf, "%s\n", &serial_string[0]);
}



/**
 * Read device file "get_firmware_version"
 * Gets the firmware version from the device,
 * Returns a string.
 */
static ssize_t razer_attr_read_get_firmware_version(struct device *dev,
    struct device_attribute *attr, char *buf)
{
    char fw_string[100] = "";
    struct usb_interface *intf = to_usb_interface(dev->parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);

    razer_get_firmware_version(usb_dev, &fw_string[0]);
    return sprintf(buf, "%s\n", &fw_string[0]);
}



/**
 * Read device file "device_type"
 * Returns a friendly device type string.
 */
static ssize_t razer_attr_read_device_type(struct device *dev,
    struct device_attribute *attr, char *buf)
{
    struct usb_interface *intf = to_usb_interface(dev->parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);

    char *device_type;

    switch (usb_dev->descriptor.idProduct)
    {
        case USB_DEVICE_ID_RAZER_BLADE_STEALTH_2016:
            device_type = "Razer Blade Stealth 2016\n";
            break;

        case USB_DEVICE_ID_RAZER_BLADE_14_2016:
            device_type = "Razer Blade 14 2016\n";
            break;

        default:
            device_type = "Unknown Device\n";
    }

    return sprintf(buf, device_type);
}



/*
 * Read device file "brightness"
 * Returns the brightness value from 0-255.
 */
static ssize_t razer_attr_read_brightness(struct device *dev,
    struct device_attribute *attr, char *buf)
{
    struct usb_interface *intf = to_usb_interface(dev->parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);

    int brightness = razer_get_brightness(usb_dev);
    return sprintf(buf, "%d\n", brightness);
}



/*
 * Write device file "rightness"
 * Sets the brightness to the ASCII number written to this file. Values from 0-255
 */
static ssize_t razer_attr_write_brightness(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    struct usb_interface *intf = to_usb_interface(dev->parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    int temp = simple_strtoul(buf, NULL, 10);
    razer_set_brightness(usb_dev, (unsigned char)temp);
    return count;
}



/**
 * Write device file "set_logo"
 * Sets the logo lighting state to the ASCII number written to this file.
 */
static ssize_t razer_attr_write_set_logo(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    struct usb_interface *intf = to_usb_interface(dev->parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    int state = simple_strtoul(buf, NULL, 10);
    razer_set_logo(usb_dev, (unsigned char)state);
    return count;
}



/*
 * Write device file "set_fn_toggle"
 * Sets the logo lighting state to the ASCII number written to this file.
 * If 0 should mean that the F-keys work as normal F-keys
 * If 1 should mean that the F-keys act as if the FN key is held
 */
static ssize_t razer_attr_write_set_fn_toggle(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    struct usb_interface *intf = to_usb_interface(dev->parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    int state = simple_strtoul(buf, NULL, 10);
    razer_set_fn_toggle(usb_dev, (unsigned char)state);
    return count;
}



//######################################//
//### Set up the device driver files ###//
//######################################//

/*
 * Read only is 0444
 * Write only is 0220
 * Read and write is 0664
 */

static DEVICE_ATTR(get_serial,              0444, razer_attr_read_get_serial,           NULL);
static DEVICE_ATTR(get_firmware_version,    0444, razer_attr_read_get_firmware_version, NULL);
static DEVICE_ATTR(device_type,             0444, razer_attr_read_device_type,          NULL);
static DEVICE_ATTR(brightness,              0660, razer_attr_read_brightness, razer_attr_write_brightness);

static DEVICE_ATTR(set_logo,        0220, NULL, razer_attr_write_set_logo);
static DEVICE_ATTR(set_fn_toggle,   0220, NULL, razer_attr_write_set_fn_toggle);



//#############################//
//### Driver Main Functions ###//
//#############################//

/*
 * Probe method is ran whenever a device is bound to the driver
 */
static int razer_kbd_probe(struct hid_device *hdev,
             const struct hid_device_id *id)
{
    int retval;
    struct usb_interface *intf = to_usb_interface(hdev->dev.parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    struct razer_kbd_device *dev = NULL;

    dev = kzalloc(sizeof(struct razer_kbd_device), GFP_KERNEL);
    if(!dev) {
        dev_err(&intf->dev, "failed allocating device descriptor\n");
        return -ENOMEM;
    }

    // Default files
    retval = device_create_file(&hdev->dev, &dev_attr_get_serial);
    if (retval)
        goto exit_free;
    retval = device_create_file(&hdev->dev, &dev_attr_get_firmware_version);
    if (retval)
        goto exit_free;
    retval = device_create_file(&hdev->dev, &dev_attr_device_type);
    if (retval)
        goto exit_free;
    retval = device_create_file(&hdev->dev, &dev_attr_brightness);
    if (retval)
        goto exit_free;


    // Custom files depending on the device support.
    if(usb_dev->descriptor.idProduct == USB_DEVICE_ID_RAZER_BLADE_STEALTH_2016 ||
            usb_dev->descriptor.idProduct == USB_DEVICE_ID_RAZER_BLADE_14_2016)
    {
        retval = device_create_file(&hdev->dev, &dev_attr_set_logo);
        if (retval)
            goto exit_free;
        retval = device_create_file(&hdev->dev, &dev_attr_set_fn_toggle);
        if (retval)
            goto exit_free;
    }


    hid_set_drvdata(hdev, dev);

    retval = hid_parse(hdev);
    if(retval)    {
        hid_err(hdev, "parse failed\n");
        goto exit_free;
    }
    retval = hid_hw_start(hdev, HID_CONNECT_DEFAULT);
    if (retval) {
        hid_err(hdev, "hw start failed\n");
        goto exit_free;
    }

    razer_reset(usb_dev);
    usb_disable_autosuspend(usb_dev);

    return 0;
exit_free:
    kfree(dev);
    return retval;
}



/*
 * Driver unbind function
 */
static void razer_kbd_disconnect(struct hid_device *hdev)
{
    struct razer_kbd_device *dev;
    struct usb_interface *intf = to_usb_interface(hdev->dev.parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);

    dev = hid_get_drvdata(hdev);
    if(!dev)    {
        dev_err(&intf->dev, "disconnect failed: failed to obtain device descriptor\n");
        return;
    }

    // Remove the default files
    device_remove_file(&hdev->dev, &dev_attr_get_firmware_version);
    device_remove_file(&hdev->dev, &dev_attr_get_serial);
    device_remove_file(&hdev->dev, &dev_attr_device_type);
    device_remove_file(&hdev->dev, &dev_attr_brightness);


    // Custom files depending on the device support.
    if(usb_dev->descriptor.idProduct == USB_DEVICE_ID_RAZER_BLADE_STEALTH_2016 ||
            usb_dev->descriptor.idProduct == USB_DEVICE_ID_RAZER_BLADE_14_2016)
    {
        device_remove_file(&hdev->dev, &dev_attr_set_logo);
        device_remove_file(&hdev->dev, &dev_attr_set_fn_toggle);
    }


    hid_hw_stop(hdev);
    kfree(dev);
    dev_info(&intf->dev, "Razer Device disconnected\n");
}



//###############################//
//### Device ID mapping table ###//
//###############################//

static const struct hid_device_id razer_devices[] = {
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER, USB_DEVICE_ID_RAZER_BLADE_STEALTH_2016) },
    { HID_USB_DEVICE(USB_VENDOR_ID_RAZER, USB_DEVICE_ID_RAZER_BLADE_14_2016) },
    { }
};

MODULE_DEVICE_TABLE(hid, razer_devices);



//############################################//
//### Describes the contents of the driver ###//
//############################################//

static struct hid_driver razer_kbd_driver = {
    .name       = "razer-kbd",
    .id_table   = razer_devices,
    .probe      = razer_kbd_probe,
    .remove     = razer_kbd_disconnect
};

module_hid_driver(razer_kbd_driver);
