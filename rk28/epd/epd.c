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

#define LOG_TAG "EpdHalStub"
#define LOCAL_LOGD 1
#include <hardware/epd.h>


int epd_device_close(struct hw_device_t* device)
{
	struct epd_control_device_t* ctx = (struct epd_control_device_t*)device;
	if (LOCAL_LOGD) {
		LOGD("Trying to close the device");
	}
	if (ctx) {
		close(ctx->fd);
		free(ctx);
	}
	return 0;
}

int epd_mode_select(struct epd_control_device_t *dev, int32_t mode)
{
	if (dev == NULL)
		return -EINVAL;
	
	/* ioctl to fb driver, select new work mode */
	switch (mode) {
		case EPD_FULL : 
		case EPD_AUTO :
		case EPD_TEXT :
		case EPD_AUTO_PART :
		case EPD_AUTO_BLACK_WHITE :
		case EPD_A2 :
			if (ioctl(dev->fd, FB0_IOCTL_SET_MODE, mode) == -1) {
				LOGE("ioctl failed while trying to set mode to : %d", mode);
				return -EIO;
			}
			break;
		default :
			LOGE("Unrecognized work mode");
			return -EINVAL;
	}
	if (LOCAL_LOGD) {
		LOGD("Work mode switched to %d successfully", mode);
	}
	return 0;
}

int epd_repaint_display(struct epd_control_device_t *dev, int32_t mode)
{
	if (dev == NULL)
		return -EINVAL;
	
	/* ioctl to fb driver, repaint the display with given work mode */
	switch (mode) {
		case EPD_FULL : 
		case EPD_AUTO :
		case EPD_TEXT :
		case EPD_AUTO_PART :
		case EPD_AUTO_BLACK_WHITE :
		case EPD_A2 :
			if (ioctl(dev->fd, FB0_IOCTL_REPAN_DISP, mode) == -1) {
				LOGE("ioctl failed while trying to repaint with mode %d", mode);
				return -EIO;
			}
			break;
		default :
			LOGE("Unrecongnized work mode");
			return -EINVAL;
	}
	if (LOCAL_LOGD) {
		LOGD("Display repainted with mode %d", mode);
	}
	return 0;
}

int epd_reset_display(struct epd_control_device_t *dev)
{
	if (dev == NULL)
		return -EINVAL;
	
	/* ioctl to fb driver, do reset */
	if (ioctl(dev->fd, FB0_IOCTL_RESET) == -1) {
		LOGE("ioctl failed while trying to reset display");
		return -EIO;
	}

	if (LOCAL_LOGD) {
		LOGD("Display reset successfully");
	}
	return 0;
}

int epd_get_status(struct epd_control_device_t *dev)
{
	if (dev == NULL)
		return -EINVAL;
	
	/* ioctl to fb driver, check status */
	int res = ioctl(dev->fd, FB0_IOCTL_GET_STATUS);
	
	if (res == 1) {
		if (LOCAL_LOGD) {
			LOGD("EPD is still busy painting");
		}
	} else if (res == 0) {
		if (LOCAL_LOGD) {
			LOGD("EPD is not busy");
		}		
	} else {
		LOGE("ioctl failed while trying to get status");
		return -EIO;
	}
	
	return res;
}



int epd_device_open(const struct hw_module_t* module, const char* name, struct hw_device_t** device) 
{
	struct epd_control_device_t *dev;

	dev = (struct epd_control_device_t *)malloc(sizeof(*dev));
	memset(dev, 0, sizeof(*dev));

	dev->common.tag =  HARDWARE_DEVICE_TAG;
	dev->common.version = 0;
	dev->common.module = (struct hw_module_t*)module;
	dev->common.close = epd_device_close;

	dev->mode_select = epd_mode_select;
	dev->repaint_display = epd_repaint_display;
	dev->reset_display = epd_reset_display;
	dev->get_status = epd_get_status;

	*device = &dev->common;

	/**
 	 * Initialize epd hardware here.
 	 */
	dev->fd = open(FB_DEVICE, O_RDWR);
	if (dev->fd < 0) {
		LOGE("Unable to open %s. Make sure you have permission", FB_DEVICE);
		return -EACCES;
	}
	if (LOCAL_LOGD) {
		LOGD("Successfully opened %s, read/write", FB_DEVICE);
	}
	return 0;
}

struct hw_module_methods_t epd_module_methods = {
	open: epd_device_open
};

const struct epd_module_t HAL_MODULE_INFO_SYM = {
	common: {
		tag: HARDWARE_MODULE_TAG,
		version_major: 2,
		version_minor: 1,
		id: EPD_HARDWARE_MODULE_ID,
    	name: "Rockchip EPD HAL Stub",
    	author: "yuzhe@rock-chips.com",
    	methods: &epd_module_methods,
    }
    /* supporting APIs go here. */
};

