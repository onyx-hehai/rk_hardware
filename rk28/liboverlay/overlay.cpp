/*
 * Copyright (C) 2008 The Android Open Source Project
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

#define LOG_TAG "RKOverlay"

#include <hardware/hardware.h>
#include <hardware/overlay.h>


#include <semaphore.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cutils/log.h>
#include <cutils/ashmem.h>
#include <cutils/atomic.h>
#include <linux/fb.h>
/*****************************************************************************/

#define LOG_FUNCTION_NAME    //LOGD(" %s ###### Calling %s() ######",  __FILE__,  __FUNCTION__);

//#define LOGD(msg...)
//#define LOGI(msg...)

struct overlay_control_context_t {
    struct overlay_control_device_t device;
    /* our private state goes below here */
    struct overlay_t* overlay_video;
};

struct overlay_data_context_t {
    struct overlay_data_device_t device;
    /* our private state goes below here */
    int ctl_fd;
    int buffer_size;
    int width;
    int height;
    int format;
    int num_buffers;
    size_t *buffers_len;
    void **buffers;
};

typedef struct
{
    uint32_t cropX;
    uint32_t cropY;
    uint32_t cropW;
    uint32_t cropH;

    uint32_t posX;
    uint32_t posY;
    uint32_t posW;
    uint32_t posH;
    uint32_t rotation;
} overlay_ctrl_t;


static int overlay_device_open(const struct hw_module_t* module, const char* name,
        struct hw_device_t** device);

static struct hw_module_methods_t overlay_module_methods = {
    open: overlay_device_open
};

struct overlay_module_t HAL_MODULE_INFO_SYM = {
    common: {
        tag: HARDWARE_MODULE_TAG,
        version_major: 1,
        version_minor: 0,
        id: OVERLAY_HARDWARE_MODULE_ID,
        name: "RK28xx Overlay module",
        author: "The Android Open Source Project",
        methods: &overlay_module_methods,
    }
};

static int overlay_count = 0;

/*****************************************************************************/

/*
 * This is the overlay_t object, it is returned to the user and represents
 * an overlay.
 * This handles will be passed across processes and possibly given to other
 * HAL modules (for instance video decode modules).
 */
struct handle_t : public native_handle {
    /* add the data fields we need here, for instance: */
    int ctl_fd;
    int width;
    int height;
    int format;
    int num_buffers;
    int buffer_size;
};

static int handle_ctl_fd(const overlay_handle_t overlay) {
    return static_cast<const struct handle_t *>(overlay)->ctl_fd;
}



static int handle_num_buffers(const overlay_handle_t overlay) {
    return static_cast<const struct handle_t *>(overlay)->num_buffers;
}

static int handle_width(const overlay_handle_t overlay) {
    return static_cast<const struct handle_t *>(overlay)->width;
}

static int handle_height(const overlay_handle_t overlay) {
    return static_cast<const struct handle_t *>(overlay)->height;
}

static int handle_format(const overlay_handle_t overlay) {
    return static_cast<const struct handle_t *>(overlay)->format;
}

static int handle_buffer_size(const overlay_handle_t overlay) {
    return static_cast<const struct handle_t *>(overlay)->buffer_size;
}

class overlay_object : public overlay_t {

    handle_t mHandle;
    overlay_ctrl_t    mCtlStage;
    overlay_ctrl_t    mCtl;

    static overlay_handle_t getHandleRef(struct overlay_t* overlay) {
        /* returns a reference to the handle, caller doesn't take ownership */
        return &(static_cast<overlay_object *>(overlay)->mHandle);
    }

public:
    overlay_object(int ctl_fd, int w, int h, int format, int num_buffers, int buffer_size)
    {
        this->overlay_t::getHandleRef = getHandleRef;
        mHandle.version     = sizeof(native_handle);
        mHandle.numFds      = 1;
        mHandle.numInts     = 5; // extra ints we have in our handle
        mHandle.ctl_fd      = ctl_fd;
        mHandle.width       = w;
        mHandle.height      = h;
        mHandle.format      = format;
        mHandle.num_buffers = num_buffers;
        mHandle.buffer_size = buffer_size;
        this->w = w;
        this->h = h;
        this->format = format;
        this->w_stride = w;
        this->h_stride = h;
    }

    int     ctl_fd()    { return mHandle.ctl_fd; }
    int     buffer_size()    { return mHandle.buffer_size; }
    int     ctl_w()    { return mHandle.width; }
    int     ctl_h()    { return mHandle.height; }
    int     ctl_fmt()    { return mHandle.format; }
    overlay_ctrl_t*   staging()   { return &mCtlStage; }
    overlay_ctrl_t*   data()       { return &mCtl; }
};

// ****************************************************************************
// Control module
// ****************************************************************************

static int overlay_get(struct overlay_control_device_t *dev, int name) {
    LOG_FUNCTION_NAME

    int result = -1;
    switch (name) {
        case OVERLAY_MINIFICATION_LIMIT:
            result = 0; // 0 = no limit
            break;
        case OVERLAY_MAGNIFICATION_LIMIT:
            result = 0; // 0 = no limit
            break;
        case OVERLAY_SCALING_FRAC_BITS:
            result = 0; // 0 = infinite
            break;
        case OVERLAY_ROTATION_STEP_DEG:
            result = 90; // 90 rotation steps (for instance)
            break;
        case OVERLAY_HORIZONTAL_ALIGNMENT:
            result = 1; // 1-pixel alignment
            break;
        case OVERLAY_VERTICAL_ALIGNMENT:
            result = 1; // 1-pixel alignment
            break;
        case OVERLAY_WIDTH_ALIGNMENT:
            result = 1; // 1-pixel alignment
            break;
        case OVERLAY_HEIGHT_ALIGNMENT:
            result = 1; // 1-pixel alignment
            break;
    }
    return result;
}


static overlay_t* overlay_createOverlay(struct overlay_control_device_t *dev,
         uint32_t w, uint32_t h, int32_t format)
{
    LOG_FUNCTION_NAME
    LOGD("  <%s> __ %d,    overlay_count = %d", __FUNCTION__, __LINE__, overlay_count);
    while(overlay_count >= 1){
		LOGD("  <%s> __ %d,    overlay_count = %d", __FUNCTION__, __LINE__, overlay_count);
    }
    overlay_object *overlay = NULL;
    overlay_control_context_t *ctx = (overlay_control_context_t *)dev;
    overlay_ctrl_t *stage;
    overlay_ctrl_t *data;

    char const * const device_template[] = {
            "/dev/graphics/fb%u",
            "/dev/fb%u",
            0 };
    int fd = -1, i = 0;
    char name[64];


    if (ctx->overlay_video) {
        LOGE("HW overlay already in use.\n");
        return NULL;
    }

    LOGI("Create overlay, w=%d h=%d format=%d\n", w, h, format);

    while ((fd==-1) && device_template[i]) {
        snprintf(name, 64, device_template[i], 1);
        fd = open(name, O_RDWR, 0);
        i++;
    }

    if (fd < 0)
        return NULL;

    if ( (overlay = new overlay_object(fd, w, h, format, 1, 16)) == NULL )
    {
        LOGE("Failed to create overlay object\n");
        close( fd );
	return NULL;
    }

    ctx->overlay_video = overlay;

    stage = overlay->staging();
    data  = overlay->data();

    stage->cropX = 0;
    stage->cropY = 0;
    stage->cropW = w;
    stage->cropH = h;
    stage->posX = 0;
    stage->posY = 0;
    stage->posW = 0;
    stage->posH = 0;
    stage->rotation = 0;

    *data = *stage;

    /* Create overlay object. Talk to the h/w here and adjust to what it can
    * do. the overlay_t returned can  be a C++ object, subclassing overlay_t
    * if needed.
    *
    * we probably want to keep a list of the overlay_t created so they can
    * all be cleaned up in overlay_close().
    */
    overlay_count ++;
    LOGD("  <%s> __ %d,    overlay_count = %d", __FUNCTION__, __LINE__, overlay_count);
    return ( overlay );
}


static void overlay_destroyOverlay(struct overlay_control_device_t *dev,
         overlay_t* overlay)
{
    LOG_FUNCTION_NAME
    LOGD("  <%s> __ %d,    overlay_count = %d", __FUNCTION__, __LINE__, overlay_count);
    overlay_control_context_t *ctx = (overlay_control_context_t *)dev;
    overlay_object *obj = static_cast<overlay_object *>(overlay);
    overlay_ctrl_t *data  = obj->data();

    int rc;
    int fd = obj->ctl_fd();

    /* 视频退出后,需要切回竖屏方向 */
    if(0!=data->rotation) {
        if (ioctl(fd, 0x5003, 0)<0) {
            LOGE("Can't set scan direction(%d)\n", 0);
        }
        data->rotation = 0;
    }

    if ( (rc = close(fd)) != 0 )
    {
        LOGE( "Error closing overlay fd/%d\n", rc );
    }

    if ( overlay && ctx->overlay_video )
    {
        if (ctx->overlay_video == overlay)
        ctx->overlay_video = NULL;
    	 LOGD("  <%s> __ %d,    overlay_count = %d", __FUNCTION__, __LINE__, overlay_count);
	 if(overlay_count > 0)
	 	overlay_count --;
        /* free resources associated with this overlay_t */
        delete overlay;
    }
}


static int overlay_setPosition(struct overlay_control_device_t *dev,
         overlay_t* overlay, int x, int y, uint32_t w, uint32_t h)
{
    LOG_FUNCTION_NAME
    LOGD("overlay_setPosition (x=%d y=%d w=%d h=%d) \n", x, y, w, h);

    overlay_object *obj = static_cast<overlay_object *>(overlay);
    overlay_ctrl_t *stage  = obj->staging();

    stage->posX = x;
    stage->posY = y;
    stage->posW = w;
    stage->posH = h;

    return 0;
}


static int overlay_getPosition(struct overlay_control_device_t *dev,
         overlay_t* overlay, int* x, int* y, uint32_t* w, uint32_t* h)
{
    LOG_FUNCTION_NAME
    overlay_object *obj = static_cast<overlay_object *>(overlay);
    overlay_ctrl_t *data  = obj->data();

    *x = (int)data->posX;
    *y = (int)data->posY;
    *w = (int)data->posW;
    *h = (int)data->posH;

    return 0;
}


static int overlay_setParameter(struct overlay_control_device_t *dev,
         overlay_t* overlay, int param, int value)
{
    int ret = -EINVAL;
    overlay_ctrl_t *stage = static_cast<overlay_object *>(overlay)->staging();

    /* set this overlay's parameter (talk to the h/w) */
    switch (param) {
        case OVERLAY_ROTATION_DEG:
            /* if only 90 rotations are supported, the call fails
             * for other values */
            LOGD("overlay_setParameter  --- Set Rotation(%d) - Not Implemented Yet!", value);
            break;
        case OVERLAY_DITHER:
            LOGD("overlay_setParameter  --- Set Dither(%d) - Not Implemented Yet!", value);
            break;
        case OVERLAY_TRANSFORM:
            LOGD("overlay_setParameter  --- Set Transform(%d)!", value);
            switch(value) {
            case HAL_TRANSFORM_ROT_90:      stage->rotation = 90;   break;
            case HAL_TRANSFORM_ROT_180:     stage->rotation = 180;  break;
            case HAL_TRANSFORM_ROT_270:     stage->rotation = 270;  break;
            default:                        stage->rotation = 0;    break;
            }
            break;
        default:
            ret = -EINVAL;
            break;
    }
    return ret;
}


int overlay_commit(struct overlay_control_device_t *dev, overlay_t* overlay)
{
    LOG_FUNCTION_NAME;
    overlay_object *obj = static_cast<overlay_object *>(overlay);
    overlay_ctrl_t *stage  = obj->staging();
    overlay_ctrl_t *data  = obj->data();

    static struct fb_var_screeninfo var;
    int fd = obj->ctl_fd();
    int ret = 0, fmt;
    uint32_t x, y, w, h;
    uint32_t panelsize[2];

    /*switch (obj->ctl_fmt()) {
        case OVERLAY_FORMAT_RGBX_8888:      fmt = 0;    break;
        case OVERLAY_FORMAT_RGB_565:        fmt = 1;    break;
        case OVERLAY_FORMAT_YCbCr_422_SP:   fmt = 2;    break;
        case OVERLAY_FORMAT_YCbCr_420_SP:   fmt = 3;    break;
        case OVERLAY_FORMAT_YCbCr_420_I:    fmt = 3;    break;  // 4
        case OVERLAY_FORMAT_CbYCrY_420_I:   fmt = 5;    break;
        default:
            LOGE("overlay_commit : Error format.\n");
            return NULL;
    }*/
    
    switch (obj->ctl_fmt()) {
        case OVERLAY_FORMAT_RGBX_8888:      fmt = 0;    break;
        case OVERLAY_FORMAT_RGB_565:        fmt = 0;    break;
        case OVERLAY_FORMAT_YCbCr_422_SP:   fmt = 1;    break;
        case OVERLAY_FORMAT_YCbCr_420_SP:   fmt = 2;    break;
        case OVERLAY_FORMAT_YCbCr_420_I:    fmt = 3;    break;  // 4
        default:
            LOGE("overlay_commit : Error format.\n");
            return NULL;
    }

    if (data->posX == stage->posX && data->posY == stage->posY &&
        data->posW == stage->posW && data->posH == stage->posH &&
        data->rotation == stage->rotation) {
        LOGI("Nothing to do!\n");
        return 0;
    }

    if(data->rotation != stage->rotation) {
        LOGD("data->rotation = %d, stage->rotation = %d", data->rotation, stage->rotation);
        if(ioctl(fd, 0x5003, stage->rotation)<0) {
            LOGE("Can't set scan direction(%d)\n", stage->rotation);
            return -EINVAL;
        }
        data->rotation = stage->rotation;
        LOGD("Can set scan direction(%d)\n", stage->rotation);
    }


    if(ioctl(fd, 0x5001, panelsize) < 0) {
        LOGE("overlay_commit : Failed to get panel size \n");
        return NULL;
    }

    if (!(stage->posX == data->posX && stage->posY == data->posY &&
        stage->posW == data->posW && stage->posH == data->posH))
    {

        switch(stage->rotation)
        {
        case 90:
        case 270:
            x = stage->posY;    y = stage->posX;
            w = stage->posH;    h = stage->posW;
            break;
        case 180:
        default:
            x = stage->posX;    y = stage->posY;
            w = stage->posW;    h = stage->posH;
            break;
        }

        if (((x + w) > panelsize[0]) || ((y + h) > panelsize[1]))
        {
            LOGE("overlay_commit : Set Position - Parameters Error.\n");
            return -1;
        }

        if (ioctl(fd, FBIOGET_VSCREENINFO, &var) == -1) {
            LOGE("ioctl fb1 FBIOGET_VSCREENINFO fail!\n");
            return -EPERM;
        }

        var.xres_virtual = (stage->cropW + 15) & 0xfffffff0;    //win0 memery x size
        var.yres_virtual = (stage->cropH + 15) & 0xfffffff0;    //win0 memery y size
        var.xoffset = stage->cropX;   //win0 start x in memery
        var.yoffset = stage->cropY;   //win0 start y in memery
        var.xres = stage->cropW;    //win0 show x size
        var.yres = stage->cropH;    //win0 show y size
        var.nonstd = ((y<<20)&0xfff00000) + ((x<<8)&0xfff00) + fmt; //win0 ypos & xpos & format (ypos<<20 + xpos<<8 + format)
        var.grayscale = ((h<<20)&0xfff00000) + ((w<<8)&0xfff00) + 0;   //win0 xsize & ysize
        var.bits_per_pixel = 16;
		
        //var.reserved[2] = 0;    //disable win0
        //var.reserved[3] = (unsigned long)(-1);
        //var.reserved[4] = (unsigned long)(-1);

        if (ioctl(fd, FBIOPUT_VSCREENINFO, &var) == -1) {
            LOGE("ioctl fb1 FBIOPUT_VSCREENINFO fail!\n");
            return -EPERM;
        }

        data->posX = stage->posX;
        data->posY = stage->posY;
        data->posW = stage->posW;
        data->posH = stage->posH;
    }

    return ret;
}


int overlay_stage(struct overlay_control_device_t *dev, overlay_t* overlay)
{
    LOG_FUNCTION_NAME
    return ( 0 );
}


static int overlay_control_close(struct hw_device_t *dev)
{
    LOG_FUNCTION_NAME
    struct overlay_control_context_t* ctx = (struct overlay_control_context_t*)dev;
    if (ctx) {
        /* free all resources associated with this device here
         * in particular the overlay_handle_t, outstanding overlay_t, etc...
         */
        free(ctx);
    }
    return 0;
}


// ****************************************************************************
// Data module
// ****************************************************************************
int overlay_initialize(struct overlay_data_device_t *dev,
        overlay_handle_t handle)
{
    LOG_FUNCTION_NAME

    struct overlay_data_context_t* ctx = (struct overlay_data_context_t*)dev;


    ctx->num_buffers  = 0;
    ctx->width        = handle_width(handle);
    ctx->height       = handle_height(handle);
    ctx->format       = handle_format(handle);
    ctx->ctl_fd       = handle_ctl_fd(handle);
    ctx->buffer_size  = handle_buffer_size(handle);
    ctx->buffers      = new void* [1];


    LOGI("Overlay initialize/width=%d/height=%d/ctx->buffers[0]=%08lx\n", ctx->width, ctx->height, (unsigned long)(ctx->buffers[0]));

    /*
     * overlay_handle_t should contain all the information to "inflate" this
     * overlay. Typically it'll have a file descriptor, informations about
     * how many buffers are there, etc...
     * It is also the place to mmap all buffers associated with this overlay
     * (see getBufferAddress).
     *
     * NOTE: this function doesn't take ownership of overlay_handle_t
     *
     */

    return 0;
}


int overlay_dequeueBuffer(struct overlay_data_device_t *dev,
              overlay_buffer_t* buf)
{
    //LOG_FUNCTION_NAME
    /* blocks until a buffer is available and return an opaque structure
     * representing this buffer.
     */
    return -EINVAL;
}


int overlay_queueBuffer(struct overlay_data_device_t *dev,
        overlay_buffer_t buffer)
{
    struct overlay_data_context_t* ctx = (struct overlay_data_context_t*)dev;
    int y_plane_size = ((ctx->width + 15) & 0xfffffff0) * ((ctx->height + 15) & 0xfffffff0);
    //LOGD("<%s>     width = %d, height = %d", __FUNCTION__, ctx->width, ctx->height);
    int data[2];

    //LOGI("overlay_queueBuffer/width=%d/height=%d/ctx->buffers[0]=%08lx\n",
    //    ctx->width, ctx->height, (unsigned long)(ctx->buffers[0]));

    ctx->num_buffers = 1;

    data[0] = *(int *)buffer;
    data[1] = (int)(data[0] + y_plane_size);
    //LOGD("<%s>   data[0] = 0x%x, data[1] = 0x%x", __FUNCTION__, data[0], data[1]);
    if (ioctl(ctx->ctl_fd, 0x5002, data) == -1) {
        LOGE("ioctl fb1 0x5002 fail!\n");
        return -1;
    }

    /* Mark this buffer for posting and recycle or free overlay_buffer_t. */
    return 0;
}


void *overlay_getBufferAddress(struct overlay_data_device_t *dev,
        overlay_buffer_t buffer)
{
    LOG_FUNCTION_NAME

    /* this may fail (NULL) if this feature is not supported. In that case,
     * presumably, there is some other HAL module that can fill the buffer,
     * using a DSP for instance
     */

    struct overlay_data_context_t* ctx = (struct overlay_data_context_t*)dev;
    void *p = NULL;
    p = ctx->buffers[0];
    LOGI("overlay_getBufferAddress/ctx->buffers[0]=%08lx/p=%08lx.\n", (unsigned long)(ctx->buffers[0]), (unsigned long)p);
    return ( p );

    /* this may fail (NULL) if this feature is not supported. In that case,
     * presumably, there is some other HAL module that can fill the buffer,
     * using a DSP for instance */
    //return NULL;
}


/* can be called to change the width and height of the overlay. */
int overlay_resizeInput(struct overlay_data_device_t *dev, uint32_t w, uint32_t h)
{
    LOG_FUNCTION_NAME
    return -EINVAL;
}


int overlay_setCrop(struct overlay_data_device_t *dev, uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
    LOG_FUNCTION_NAME
    LOGD("  <%s> __ %d,  0x5008,   0x%x,  %d, %d, 0x%x", __FUNCTION__, __LINE__, x, y, w, h);
    struct overlay_data_context_t* ctx = (struct overlay_data_context_t*)dev;
    //if(x == -1)
    {	//set new FB
    	LOGD("  <%s> __ %d,  0x5008,   0x%x,  %d, %d, 0x%x", __FUNCTION__, __LINE__, x, y, w, h);
    	uint32_t array[4] = {x, y, w, h};
	if (ioctl(ctx->ctl_fd, 0x5008, array) == -1) {
		LOGE("ioctl fb1 0x5008 fail!\n");
		return -EPERM;
    	}   
    }
    return 0;
}


int overlay_getCrop(struct overlay_data_device_t *dev, uint32_t* x, uint32_t* y, uint32_t* w, uint32_t* h)
{
    LOG_FUNCTION_NAME
    if((sizeof (x) / sizeof (x[0]) ) <2){
		LOGD(" ============__%d", __LINE__);
    }
	
    LOGD("   <%s> __ %d,   (%d, %d)   (%d, %d)     scale: (%d, %d) ", __FUNCTION__, __LINE__, x[0], y[0], x[1], y[1], *w, *h);
    int array[6] = {x[0], y[0], x[1], y[1], *w, *h};
    struct overlay_data_context_t* ctx = (struct overlay_data_context_t*)dev;
    if (ioctl(ctx->ctl_fd, 0x5005, array) == -1) {
	LOGE("ioctl fb1 0x5005 fail!\n");
	return -EPERM;
    }    
    return 0;
}


int overlay_setParameter_d(struct overlay_data_device_t *dev, int param, int value)
{
    LOG_FUNCTION_NAME
    struct overlay_data_context_t* ctx = (struct overlay_data_context_t*)dev;
    static struct fb_var_screeninfo var1;
    static struct fb_var_screeninfo var2;
    int vx = (value >> 16) & 0x0000ffff;
    int vy = value & 0x0000ffff;
    switch(param){
		case 0:		/*设置虚拟屏的大小*/			
			LOGD("  <%s> __%d,   vx = %d, vy = %d", __FUNCTION__, __LINE__, vx, vy);
			if (ioctl(ctx->ctl_fd, FBIOGET_VSCREENINFO, &var1) == -1) {
			            LOGE("ioctl fb1 FBIOGET_VSCREENINFO fail!\n");
			            return -EPERM;
		        }

		        //var1.xres_virtual = ((ctx->width + 15) & 0xfffffff0);    //win0 memery x size
        		 //var1.yres_virtual = ((ctx->height+ 15) & 0xfffffff0);    //win0 memery y size
        		 
        		 var1.xres_virtual = vx;    //win0 memery x size
        		 var1.yres_virtual = vy;    //win0 memery y size
        		 //var1.grayscale = ((vy<<20)&0xfff00000) + ((vx<<8)&0xfff00) + 0;
		        if (ioctl(ctx->ctl_fd, FBIOPUT_VSCREENINFO, &var1) == -1) {
		            LOGE("ioctl fb1 FBIOPUT_VSCREENINFO fail!\n");
		            return -EPERM;
		        }
				
			 if (ioctl(ctx->ctl_fd, FBIOGET_VSCREENINFO, &var2) == -1) {
			            LOGE("ioctl fb1 FBIOGET_VSCREENINFO fail!\n");
			            return -EPERM;
		        }
			 LOGD(" <%s> __%d,  var2.xres_virtual = %d    var2.yres_virtual = %d", __FUNCTION__, __LINE__, var2.xres_virtual, var2.yres_virtual );
			break;
			
		case 1:	/*设置显示偏长移的x 与y*/
			LOGD("  <%s> __%d,   vx = %d, vy = %d", __FUNCTION__, __LINE__, vx, vy);
			/*
			if (ioctl(ctx->ctl_fd, FBIOGET_VSCREENINFO, &var1) == -1) {
			            LOGE("ioctl fb1 FBIOGET_VSCREENINFO fail!\n");
			            return -EPERM;
		        }
			var1.xoffset = vx;   //win0 start x in memery
			var1.yoffset = vy;   //win0 start y in memery
			 if (ioctl(ctx->ctl_fd, FBIOPUT_VSCREENINFO, &var1) == -1) {
		            LOGE("ioctl fb1 FBIOPUT_VSCREENINFO fail!\n");
		            return -EPERM;
		        }
			 */
			 if (ioctl(ctx->ctl_fd, 0x5006, &value) == -1) {
		            LOGE("ioctl fb1 0x5006 value = %d,  fail!\n", value);
		            return -EPERM;
		        }
			break;
		case 2:	/*设置fb的特殊状态*/
			if (ioctl(ctx->ctl_fd, 0x5007, &value) == -1) {
		            LOGE("ioctl fb1 0x5007 value = %d,  fail!\n", value);
		            return -EPERM;
		        }
			break;
		case 3:
			LOGD("  <%s> __%d, 0x5008,   value = %d", __FUNCTION__, __LINE__, value);
			if (ioctl(ctx->ctl_fd, 0x5008, &value) == -1) {
		            LOGE("ioctl fb1 0x5008 value = %d,  fail!\n", value);
		            return -EPERM;
		        }
			break;
		case 4:
			LOGD("  <%s> __%d,  0x5009,  value = %d", __FUNCTION__, __LINE__, value);
			if (ioctl(ctx->ctl_fd, 0x5009, &value) == -1) {
		            LOGE("ioctl fb1 0x5009 value = %d,  fail!\n", value);
		            return -EPERM;
		        }
			break;
    }
    return 0;
}


static int overlay_data_close(struct hw_device_t *dev)
{
    LOG_FUNCTION_NAME
    struct overlay_data_context_t* ctx = (struct overlay_data_context_t*)dev;
    if (ctx) {

        delete(ctx->buffers);
        /* free all resources associated with this device here
         * in particular all pending overlay_buffer_t if needed.
         *
         * NOTE: overlay_handle_t passed in initialize() is NOT freed and
         * its file descriptors are not closed (this is the responsibility
         * of the caller).
         */
        free(ctx);
    }
    return 0;
}


/*****************************************************************************/
static int overlay_device_open(const struct hw_module_t* module, const char* name,
        struct hw_device_t** device)
{
    int status = -EINVAL;

    LOGE("overlay_device_open/name=%s", name);

    if (!strcmp(name, OVERLAY_HARDWARE_CONTROL)) {
        struct overlay_control_context_t *dev;
        dev = (overlay_control_context_t*)malloc(sizeof(*dev));

        /* initialize our state here */
        memset(dev, 0, sizeof(*dev));

        /* initialize the procs */
        dev->device.common.tag = HARDWARE_DEVICE_TAG;
        dev->device.common.version = 0;
        dev->device.common.module = const_cast<hw_module_t*>(module);
        dev->device.common.close = overlay_control_close;

        dev->device.get = overlay_get;
        dev->device.createOverlay = overlay_createOverlay;
        dev->device.destroyOverlay = overlay_destroyOverlay;
        dev->device.setPosition = overlay_setPosition;
        dev->device.getPosition = overlay_getPosition;
        dev->device.setParameter = overlay_setParameter;
        dev->device.stage = overlay_stage;
        dev->device.commit = overlay_commit;

        *device = &dev->device.common;
        status = 0;
    } else if (!strcmp(name, OVERLAY_HARDWARE_DATA)) {
        struct overlay_data_context_t *dev;
        dev = (overlay_data_context_t*)malloc(sizeof(*dev));

        /* initialize our state here */
        memset(dev, 0, sizeof(*dev));

        /* initialize the procs */
        dev->device.common.tag = HARDWARE_DEVICE_TAG;
        dev->device.common.version = 0;
        dev->device.common.module = const_cast<hw_module_t*>(module);
        dev->device.common.close = overlay_data_close;

        dev->device.initialize = overlay_initialize;
        dev->device.dequeueBuffer = overlay_dequeueBuffer;
        dev->device.queueBuffer = overlay_queueBuffer;
        dev->device.getBufferAddress = overlay_getBufferAddress;
        dev->device.resizeInput = overlay_resizeInput;
        dev->device.setCrop = overlay_setCrop;
        dev->device.getCrop = overlay_getCrop;
        dev->device.setParameter = overlay_setParameter_d;

        *device = &dev->device.common;
        status = 0;
    }
    return status;
}


