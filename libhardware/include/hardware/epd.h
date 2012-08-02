/*
 * Copyright (C) 2010 Rockchip Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ROCKCHIP_EPD_INTERFACE_H
#define ROCKCHIP_EPD_INTERFACE_H

#include <hardware/hardware.h>

#include <fcntl.h>
#include <errno.h>

#include <cutils/log.h>
#include <cutils/atomic.h>
#include <cutils/properties.h>


/**
 * Definition of kernel-space driver.
 */
#define	FB_DEVICE	"/dev/graphics/fb0"

/* EPD work modes */
#define EPD_FULL           0 // 
#define EPD_FULL_WIN       1 // not implemented yet
#define EPD_PART           2 // obsoleted, do not use
#define EPD_PART_WIN       3 // not implemented yet
#define EPD_BLACK_WHITE    4 // obsoleted, do not use
#define EPD_AUTO           5 // default
#define EPD_DRAW_PEN	   6 // not implemented yet
#define EPD_GU_FULL		   7 // not implemented yet
#define EPD_GU_PART		   8 // not implemented yet
#define EPD_TEXT	       9
#define EPD_AUTO_PART		10
#define EPD_AUTO_BLACK_WHITE 11
#define EPD_A2				12

/* fb0 control word used by ioctl*/
#define FB0_IOCTL_SET_MODE			0x6003
#define FB0_IOCTL_REPAN_DISP		0x6004
#define FB0_IOCTL_RESET				0x6005
#define FB0_IOCTL_GET_STATUS		0x6006
#define FB0_IOCTL_GET_WAVEFORM_NUM	0x6007
#define FB0_IOCTL_SET_IDLE_TIME		0x6008

/**
 * The id of this module
 */
#define EPD_HARDWARE_MODULE_ID "epd"

/**
 * Every hardware module must have a data structure named HAL_MODULE_INFO_SYM
 * and the fields of this data structure must begin with hw_module_t
 * followed by module specific information.
 */
struct epd_module_t 
{
	struct hw_module_t common;
};

/**
 * Every device data structure must begin with hw_device_t
 * followed by module specific public methods and attributes.
 */
struct epd_control_device_t 
{
	struct hw_device_t common;

	/* file descriptor of epd device */	
	int fd;

	/* supporting control APIs go here */
	int (*mode_select)(struct epd_control_device_t *dev, int32_t mode);
	int (*repaint_display)(struct epd_control_device_t *dev, int32_t mode);
	int (*reset_display)(struct epd_control_device_t *dev);
	int (*get_status)(struct epd_control_device_t *dev);
};

#endif  // ROCKCHIP_EPD_INTERFACE_H



