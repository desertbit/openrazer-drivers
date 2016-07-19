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

#ifndef HID_RAZER_H
#define HID_RAZER_H


//#################//
//### Constants ###//
//#################//

#define RAZER_KBD_WAIT_MIN_US 600
#define RAZER_KBD_WAIT_MAX_US 800

// Device IDs
#define USB_DEVICE_ID_RAZER_BLADE_STEALTH_2016  0x0205
#define USB_DEVICE_ID_RAZER_BLADE_14_2016       0x020F

// Keyboard rows and columns.
#define RAZER_STEALTH_2016_ROWS     0x06
#define RAZER_STEALTH_2016_COLUMNS  0x10
#define RAZER_BLADE_14_2016_ROWS    0x06
#define RAZER_BLADE_14_2016_COLUMNS 0x10

#endif // HID_RAZER_H
