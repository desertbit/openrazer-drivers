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
#include "hid-razer.h"



//###########################//
//### Version Information ###//
//###########################//

MODULE_AUTHOR("Roland Singer <roland.singer@desertbit.com>");
MODULE_AUTHOR("Tim Theede <pez2001@voyagerproject.de>");
MODULE_DESCRIPTION("USB HID Razer Keyboard");
MODULE_LICENSE("GPL v2");



//########################//
//### Helper functions ###//
//########################//

// Send a report to the keyboard.
int razer_set_report(struct usb_device *usb_dev,void const *data)
{
    return razer_send_control_msg(usb_dev, data, 0x02,
        RAZER_KBD_WAIT_MIN_US, RAZER_KBD_WAIT_MAX_US);
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
    if(retval != 0) {
        razer_print_err_report(&report, "hid-razer", "failed to reset keyboard");
        return retval;
    }

    return 0;
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
        razer_print_err_report(&response_report, "hid-razer", "invalid report length");
        return retval;
    }

    if(response_report.status == 0x02 &&
        response_report.command_class == 0x00 &&
        response_report.command_id.id == 0x81)
    {
        sprintf(fw_string, "v%d.%d", response_report.arguments[0], response_report.arguments[1]);
        retval = response_report.arguments[2];
    }
    else {
        razer_print_err_report(&response_report, "hid-razer", "invalid report type");
        return -EINVAL;
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
        razer_print_err_report(&response_report, "hid-razer", "invalid report length");
        return retval;
    }

    // For the Razer Blades
    if(response_report.status == 0x02 &&
        response_report.command_class == 0x0E &&
        response_report.command_id.id == 0x84)
    {
        retval = response_report.arguments[1];
    }
    else {
        razer_print_err_report(&response_report, "hid-razer", "invalid report type");
        return -EINVAL;
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
        razer_print_err_report(&report, "hid-razer", "set_brightness: request failed");
        return retval;
    }

    return 0;
}

// Set the logo lighting state (on/off only)
int razer_set_logo(struct usb_device *usb_dev, unsigned char state)
{
    int retval;
    struct razer_report report = new_razer_report(0x03, 0x00, 0x03);

    if (state != 0 && state != 1) {
        printk(KERN_WARNING "hid-razer: set_logo: logo lighting state must be either 0 or 1: got: %d", state);
        return -EINVAL;
    }

    report.arguments[0] = 0x01; // LED Class
    report.arguments[1] = 0x04; // LED ID, Logo
    report.arguments[2] = state; // State
    report.crc = razer_calculate_crc(&report);

    retval = razer_set_report(usb_dev, &report);
    if(retval != 0) {
        razer_print_err_report(&report, "hid-razer", "set_logo: request failed");
        return retval;
    }

    return 0;
}

// Toggle FN key
int razer_set_fn_toggle(struct usb_device *usb_dev, unsigned char state)
{
    int retval;
    struct razer_report report = new_razer_report(0x02, 0x06, 0x02);

    if (state != 0 && state != 1) {
        printk(KERN_WARNING "hid-razer: fn_toggle: toggle FN state must be either 0 or 1: got: %d", state);
        return -EINVAL;
    }

    report.arguments[0] = 0x00;
    report.arguments[1] = state; // State
    report.crc = razer_calculate_crc(&report);

    retval = razer_set_report(usb_dev, &report);
    if(retval != 0) {
        razer_print_err_report(&report, "hid-razer", "fn_toggle: request failed");
        return retval;
    }

    return 0;
}

// Disable any keyboard effect
int razer_set_none_mode(struct usb_device *usb_dev)
{
    int retval;
    struct razer_report report = new_razer_report(0x03, 0x0A, 0x01);
    report.arguments[0] = 0x00; // Effect ID
    report.crc = razer_calculate_crc(&report);

    retval = razer_set_report(usb_dev, &report);
    if(retval != 0) {
        razer_print_err_report(&report, "hid-razer", "none_mode: request failed");
        return retval;
    }

    return 0;
}

// Set static effect on the keyboard
int razer_set_static_mode(struct usb_device *usb_dev, struct razer_rgb *color)
{
    int retval;
    struct razer_report report = new_razer_report(0x03, 0x0A, 0x04);
    report.arguments[0] = 0x06;     // Effect ID
    report.arguments[1] = color->r; // rgb color definition
    report.arguments[2] = color->g;
    report.arguments[3] = color->b;
    report.crc = razer_calculate_crc(&report);

    retval = razer_set_report(usb_dev, &report);
    if(retval != 0) {
        razer_print_err_report(&report, "hid-razer", "static_mode: request failed");
        return retval;
    }

    return 0;
}

// Set custom effect on the keyboard
int razer_set_custom_mode(struct usb_device *usb_dev)
{
    int retval;
    struct razer_report report = new_razer_report(0x03, 0x0A, 0x02);
    report.arguments[0] = 0x05; // Effect ID
    report.arguments[1] = 0x00; // Data frame ID
    report.crc = razer_calculate_crc(&report);

    retval = razer_set_report(usb_dev, &report);
    if(retval != 0) {
        razer_print_err_report(&report, "hid-razer", "custom_mode: request failed");
        return retval;
    }

    return 0;
}

// Set the wave effect on the keyboard
// TODO: Add posibility to set custom colors and speed...
int razer_set_starlight_mode(struct usb_device *usb_dev)
{
    int retval;
    struct razer_report report = new_razer_report(0x03, 0x0A, 0x09);
    report.arguments[0] = 0x19; // Effect ID
    report.arguments[1] = 0x01; // Type one color
    report.arguments[2] = 0x01; // Speed

    report.arguments[3] = 0x00; // Red 1
    report.arguments[4] = 0xFF; // Green 1
    report.arguments[5] = 0x00; // Blue 1

    report.arguments[6] = 0x00; // Red 2
    report.arguments[7] = 0x00; // Green 2
    report.arguments[8] = 0x00; // Blue 2

    report.crc = razer_calculate_crc(&report);

    retval = razer_set_report(usb_dev, &report);
    if(retval != 0) {
        razer_print_err_report(&report, "hid-razer", "starlight_mode: request failed");
        return retval;
    }

    return 0;
}

// Set the wave effect on the keyboard
int razer_set_wave_mode(struct usb_device *usb_dev, unsigned char direction)
{
    int retval;
    struct razer_report report = new_razer_report(0x03, 0x0A, 0x02);
    report.arguments[0] = 0x01; // Effect ID
    report.arguments[1] = direction; // Direction
    report.crc = razer_calculate_crc(&report);

    retval = razer_set_report(usb_dev, &report);
    if(retval != 0) {
        razer_print_err_report(&report, "hid-razer", "wave_mode: request failed");
        return retval;
    }

    return 0;
}

// Set spectrum effect on the keyboard
int razer_set_spectrum_mode(struct usb_device *usb_dev)
{
    int retval;
    struct razer_report report = new_razer_report(0x03, 0x0A, 0x01);
    report.arguments[0] = 0x04; // Effect ID
    report.crc = razer_calculate_crc(&report);

    retval = razer_set_report(usb_dev, &report);
    if(retval != 0) {
        razer_print_err_report(&report, "hid-razer", "spectrum_mode: request failed");
        return retval;
    }

    return 0;
}

// Set reactive effect on the keyboard
int razer_set_reactive_mode(struct usb_device *usb_dev, struct razer_rgb *color, unsigned char speed)
{
    int retval = 0;
    struct razer_report report = new_razer_report(0x03, 0x0A, 0x05);

    if (speed <= 0 || speed >= 4) {
        printk(KERN_WARNING "hid-razer: reactive_mode: speed must be within 1-3: got: %d", speed);
        return -EINVAL;
    }

    report.arguments[0] = 0x02; // Effect ID
    report.arguments[1] = speed; // Time
    report.arguments[2] = color->r; /*rgb color definition*/
    report.arguments[3] = color->g;
    report.arguments[4] = color->b;
    report.crc = razer_calculate_crc(&report);

    retval = razer_set_report(usb_dev, &report);
    if(retval != 0) {
        razer_print_err_report(&report, "hid-razer", "reactive_mode: request failed");
        return retval;
    }

    return 0;
}


// Set breath effect on the keyboard
int razer_set_breath_mode(struct usb_device *usb_dev, unsigned char breathing_type, struct razer_rgb *color1, struct razer_rgb *color2)
{
    int retval;
    struct razer_report report = new_razer_report(0x03, 0x0A, 0x08);
    report.arguments[0] = 0x03; // Effect ID

    report.arguments[1] = breathing_type;

    if(breathing_type == 1 || breathing_type == 2) {
        // Colour 1
        report.arguments[2] = color1->r;
        report.arguments[3] = color1->g;
        report.arguments[4] = color1->b;
    }

    if(breathing_type == 2) {
        // Colour 2
        report.arguments[5] = color2->r;
        report.arguments[6] = color2->g;
        report.arguments[7] = color2->b;
    }

    report.crc = razer_calculate_crc(&report);

    retval = razer_set_report(usb_dev, &report);
    if(retval != 0) {
        razer_print_err_report(&report, "hid-razer", "breath_mode: request failed");
        return retval;
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
    int retval;

    retval = razer_get_firmware_version(usb_dev, &fw_string[0]);
    if (retval != 0) {
        return retval;
    }

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
 * Write device file "brightness"
 * Sets the brightness to the ASCII number written to this file. Values from 0-255
 */
static ssize_t razer_attr_write_brightness(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    struct usb_interface *intf = to_usb_interface(dev->parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    int temp = simple_strtoul(buf, NULL, 10);
    int retval;

    retval = razer_set_brightness(usb_dev, (unsigned char)temp);
    if (retval != 0) {
        return retval;
    }

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
    int retval;

    retval = razer_set_logo(usb_dev, (unsigned char)state);
    if (retval != 0) {
        return retval;
    }

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
    int retval;

    retval = razer_set_fn_toggle(usb_dev, (unsigned char)state);
    if (retval != 0) {
        return retval;
    }

    return count;
}



/*
 * Write device file "mode_none"
 * Disable keyboard effects / turns the keyboard LEDs off.
 */
static ssize_t razer_attr_write_mode_none(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    struct usb_interface *intf = to_usb_interface(dev->parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    int retval;

    retval = razer_set_none_mode(usb_dev);
    if (retval != 0) {
        return retval;
    }

    return count;
}



/*
 * Write device file "mode_static"
 * Set the keyboard to static mode when 3 RGB bytes are written.
 */
static ssize_t razer_attr_write_mode_static(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct usb_interface *intf = to_usb_interface(dev->parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    int retval;

    if(count != 3) {
        printk(KERN_WARNING "hid-razer: mode static requires 3 RGB bytes");
        return -EINVAL;
    }

    retval = razer_set_static_mode(usb_dev, (struct razer_rgb*)&buf[0]);
    if (retval != 0) {
        return retval;
    }

    return count;
}



/*
 * Write device file "mode_custom"
 * Sets the keyboard to custom mode whenever the file is written to.
 */
static ssize_t razer_attr_write_mode_custom(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    struct usb_interface *intf = to_usb_interface(dev->parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    int retval;

    // Reset the razer keyboard. Ignore errors. This is not fatal.
    razer_reset(usb_dev);

    retval = razer_set_custom_mode(usb_dev);
    if (retval != 0) {
        return retval;
    }

    return count;
}



/*
 * Write device file "mode_startlight"
 * Starlight effect mode is activated whenever the file is written to.
 */
static ssize_t razer_attr_write_mode_starlight(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    struct usb_interface *intf = to_usb_interface(dev->parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    int retval;

    retval = razer_set_starlight_mode(usb_dev);
    if (retval != 0) {
        return retval;
    }

    return count;
}



/*
 * Write device file "mode_wave"
 * When 1 is written (as a character, 0x31) the wave effect is displayed moving left across the keyboard.
 * if 2 is written (0x32) then the wave effect goes right.
 */
static ssize_t razer_attr_write_mode_wave(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    struct usb_interface *intf = to_usb_interface(dev->parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    int temp = simple_strtoul(buf, NULL, 10);
    int retval;

    retval = razer_set_wave_mode(usb_dev, temp);
    if (retval != 0) {
        return retval;
    }

    return count;
}



/*
 * Write device file "mode_spectrum"
 * Specrum effect mode is activated whenever the file is written to.
 */
static ssize_t razer_attr_write_mode_spectrum(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    struct usb_interface *intf = to_usb_interface(dev->parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    int retval;

    retval = razer_set_spectrum_mode(usb_dev);
    if (retval != 0) {
        return retval;
    }

    return count;
}



/*
 * Write device file "mode_reactive"
 * Sets reactive mode when this file is written to. A speed byte and 3 RGB bytes should be written.
 * The speed must be within 01-03
 * 1 Short, 2 Medium, 3 Long
 */
static ssize_t razer_attr_write_mode_reactive(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    struct usb_interface *intf = to_usb_interface(dev->parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    int retval;

    if (count != 4) {
        printk(KERN_WARNING "hid-razer: mode reactive requires one speed byte followed by 3 RGB bytes");
        return -EINVAL;
    }

    retval = razer_set_reactive_mode(usb_dev, (struct razer_rgb*)&buf[1], (unsigned char)buf[0]);
    if (retval != 0) {
        return retval;
    }

    return count;
}



/*
 * Write device file "mode_breath"
 * Breathing mode has 3 modes of operation.
 * Mode 1 fading in and out using a single colour. 3 RGB bytes.
 * Mode 2 is fading in and out between two colours. 6 RGB bytes.
 * Mode 3 is fading in and out between random colours.
 */
static ssize_t razer_attr_write_mode_breath(struct device *dev,
    struct device_attribute *attr, const char *buf, size_t count)
{
    struct usb_interface *intf = to_usb_interface(dev->parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);
    int retval;

    const char *alt_buf[6] = { 0 };

    if(count == 3) {
        // Single colour mode
        retval = razer_set_breath_mode(usb_dev, 0x01, (struct razer_rgb*)&buf[0], (struct razer_rgb*)&alt_buf[3]);
        if (retval != 0) {
            return retval;
        }
    }
    else if(count == 6) {
        // Dual colour mode
        retval = razer_set_breath_mode(usb_dev, 0x02, (struct razer_rgb*)&buf[0], (struct razer_rgb*)&buf[3]);
        if (retval != 0) {
            return retval;
        }
    }
    else {
        // "Random" colour mode
        retval = razer_set_breath_mode(usb_dev, 0x03, (struct razer_rgb*)&alt_buf[0], (struct razer_rgb*)&alt_buf[3]);
        if (retval != 0) {
            return retval;
        }
    }

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

static DEVICE_ATTR(mode_none,       0220, NULL, razer_attr_write_mode_none);
static DEVICE_ATTR(mode_static,     0220, NULL, razer_attr_write_mode_static);
static DEVICE_ATTR(mode_custom,     0220, NULL, razer_attr_write_mode_custom);
static DEVICE_ATTR(mode_wave,       0220, NULL, razer_attr_write_mode_wave);
static DEVICE_ATTR(mode_spectrum,   0220, NULL, razer_attr_write_mode_spectrum);
static DEVICE_ATTR(mode_reactive,   0220, NULL, razer_attr_write_mode_reactive);
static DEVICE_ATTR(mode_breath,     0220, NULL, razer_attr_write_mode_breath);
static DEVICE_ATTR(mode_starlight,  0220, NULL, razer_attr_write_mode_starlight);



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

    // Default files
    retval = device_create_file(&hdev->dev, &dev_attr_get_serial);
    if (retval)
        return retval;
    retval = device_create_file(&hdev->dev, &dev_attr_get_firmware_version);
    if (retval)
        return retval;
    retval = device_create_file(&hdev->dev, &dev_attr_device_type);
    if (retval)
        return retval;
    retval = device_create_file(&hdev->dev, &dev_attr_brightness);
    if (retval)
        return retval;


    // Custom files depending on the device support.
    if(usb_dev->descriptor.idProduct == USB_DEVICE_ID_RAZER_BLADE_STEALTH_2016 ||
            usb_dev->descriptor.idProduct == USB_DEVICE_ID_RAZER_BLADE_14_2016)
    {
        retval = device_create_file(&hdev->dev, &dev_attr_set_logo);
        if (retval)
            return retval;
        retval = device_create_file(&hdev->dev, &dev_attr_set_fn_toggle);
        if (retval)
            return retval;

        // Modes
        retval = device_create_file(&hdev->dev, &dev_attr_mode_none);
        if (retval)
            return retval;
        retval = device_create_file(&hdev->dev, &dev_attr_mode_static);
        if (retval)
            return retval;
        retval = device_create_file(&hdev->dev, &dev_attr_mode_custom);
        if (retval)
            return retval;
        retval = device_create_file(&hdev->dev, &dev_attr_mode_wave);
        if (retval)
            return retval;
        retval = device_create_file(&hdev->dev, &dev_attr_mode_spectrum);
        if (retval)
            return retval;
        retval = device_create_file(&hdev->dev, &dev_attr_mode_reactive);
        if (retval)
            return retval;
        retval = device_create_file(&hdev->dev, &dev_attr_mode_breath);
        if (retval)
            return retval;
        retval = device_create_file(&hdev->dev, &dev_attr_mode_starlight);
        if (retval)
            return retval;
    }

    retval = hid_parse(hdev);
    if(retval)    {
        hid_err(hdev, "parse failed\n");
        return retval;
    }
    retval = hid_hw_start(hdev, HID_CONNECT_DEFAULT);
    if (retval) {
        hid_err(hdev, "hw start failed\n");
        return retval;
    }

    // Reset the razer device. Ignore errors. This is not fatal.
    razer_reset(usb_dev);

    usb_disable_autosuspend(usb_dev);

    return 0;
}



/*
 * Driver unbind function
 */
static void razer_kbd_disconnect(struct hid_device *hdev)
{
    struct usb_interface *intf = to_usb_interface(hdev->dev.parent);
    struct usb_device *usb_dev = interface_to_usbdev(intf);

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

        // Modes
        device_remove_file(&hdev->dev, &dev_attr_mode_none);
        device_remove_file(&hdev->dev, &dev_attr_mode_static);
        device_remove_file(&hdev->dev, &dev_attr_mode_custom);
        device_remove_file(&hdev->dev, &dev_attr_mode_wave);
        device_remove_file(&hdev->dev, &dev_attr_mode_spectrum);
        device_remove_file(&hdev->dev, &dev_attr_mode_reactive);
        device_remove_file(&hdev->dev, &dev_attr_mode_breath);
        device_remove_file(&hdev->dev, &dev_attr_mode_starlight);
    }


    hid_hw_stop(hdev);
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
    .name       = "hid-razer",
    .id_table   = razer_devices,
    .probe      = razer_kbd_probe,
    .remove     = razer_kbd_disconnect
};

module_hid_driver(razer_kbd_driver);
