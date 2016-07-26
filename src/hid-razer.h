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

#ifndef __HID_RAZER_H
#define __HID_RAZER_H

#include <linux/types.h>



//#################//
//### Constants ###//
//#################//

// Report indexes.
#define RAZER_DEFAULT_REPORT_INDEX  0x02

// Keyboard rows and columns:
#define RAZER_STEALTH_2016_ROWS     0x06
#define RAZER_STEALTH_2016_COLUMNS  0x10

#define RAZER_BLADE_14_2016_ROWS    0x06
#define RAZER_BLADE_14_2016_COLUMNS 0x10

#define RAZER_BLACKWIDOW_CHROMA_ROWS    0x06
#define RAZER_BLACKWIDOW_CHROMA_COLUMNS 0x16



//#############//
//### Types ###//
//#############//

struct razer_data {
    char macro_keys_state;
    char fn_toggle_state;
};

#endif // __HID_RAZER_H
