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


#include <sys/mman.h>

#include <dlfcn.h>

#include <cutils/ashmem.h>
#include <cutils/log.h>

#include <hardware/hardware.h>
#include <hardware/gralloc.h>

#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>

#include <cutils/log.h>
#include <cutils/atomic.h>

#if HAVE_ANDROID_OS
#include <linux/fb.h>
#endif

#include "gralloc_priv.h"
#include "gr.h"

#include <cutils/properties.h>

/*****************************************************************************/

// numbers of buffers for page flipping

//mod by cjf
/*
#if THREE_BUFFER
    #define NUM_BUFFERS 3
#else
    #define NUM_BUFFERS 2
#endif*/
#define NUM_BUFFERS 2


enum {
    PAGE_FLIP = 0x00000001,
    LOCKED = 0x00000002
};

struct fb_context_t {
    framebuffer_device_t  device;
};

/*****************************************************************************/

static int fb_setSwapInterval(struct framebuffer_device_t* dev,
            int interval)
{
    fb_context_t* ctx = (fb_context_t*)dev;
    if (interval < dev->minSwapInterval || interval > dev->maxSwapInterval)
        return -EINVAL;
    // FIXME: implement fb_setSwapInterval
    return 0;
}

static int fb_setUpdateRect(struct framebuffer_device_t* dev,
        int l, int t, int w, int h)
{
    if (((w|h) <= 0) || ((l|t)<0))
        return -EINVAL;
        
    fb_context_t* ctx = (fb_context_t*)dev;
    private_module_t* m = reinterpret_cast<private_module_t*>(
            dev->common.module);
    m->info.reserved[0] = 0x54445055; // "UPDT";
    m->info.reserved[1] = (uint16_t)l | ((uint32_t)t << 16);
    m->info.reserved[2] = (uint16_t)(l+w) | ((uint32_t)(t+h) << 16);
    return 0;
}

/////////////add by cjf/////////

static int gRGB565_2_Luma = 0;
static int gRotate = 0;
static int gSkipPost = 1;
static int gPixel_format_l8=0;
#define RGB2Luma_4bit(r,g,b)        ((77*r+150*g+29*b)>>12)

void RGB565_2_Luma_0(unsigned short *luma, unsigned short *Src, unsigned int width)
{
    unsigned int i;
    unsigned int r, g, b, g0, g1, g2, g3;
    unsigned int rgb_data;

    for (i=0; i<width; i++)
    {
        rgb_data = *Src++;
        r = (rgb_data & 0xf800)>>8;
        g = (rgb_data & 0x07e0)>>3;
        b = (rgb_data & 0x1f)<<3;       
        g0 = RGB2Luma_4bit(r, g, b);

        rgb_data = *Src++;      
        r = (rgb_data & 0xf800)>>8;
        g = (rgb_data & 0x07e0)>>3;
        b = (rgb_data & 0x1f)<<3;               
        g1 = RGB2Luma_4bit(r, g, b);

        rgb_data = *Src++;      
        r = (rgb_data & 0xf800)>>8;
        g = (rgb_data & 0x07e0)>>3;
        b = (rgb_data & 0x1f)<<3;               
        g2 = RGB2Luma_4bit(r, g, b);

        rgb_data = *Src++;      
        r = (rgb_data & 0xf800)>>8;
        g = (rgb_data & 0x07e0)>>3;
        b = (rgb_data & 0x1f)<<3;       
        g3 = RGB2Luma_4bit(r, g, b);
        *luma++ = (g0 & 0x0F) | ((g1 & 0xF)<<4) | ((g2 & 0xF)<<8)| ((g3 & 0xF)<<12);
    }
}

static void RGB565_2_Luma_90(unsigned short *luma, unsigned short *Src,
                unsigned int width, unsigned int height) {
        unsigned int i;
        unsigned int r, g, b, g0, g1, g2, g3;
        unsigned int rgb_data;
        for (i = 0; i < height; i += 4) {
                rgb_data = *Src;
                Src += width;
                r = (rgb_data & 0xf800) >> 8;
                g = (rgb_data & 0x07e0) >> 3;
                b = (rgb_data & 0x001f) << 3;
                g0 = RGB2Luma_4bit(r, g, b);
                rgb_data = *Src;
                Src += width;
                r = (rgb_data & 0xf800) >> 8;
                g = (rgb_data & 0x07e0) >> 3;
                b = (rgb_data & 0x001f) << 3;
                g1 = RGB2Luma_4bit(r, g, b);
                rgb_data = *Src;
                Src += width;
                r = (rgb_data & 0xf800) >> 8;
                g = (rgb_data & 0x07e0) >> 3;
                b = (rgb_data & 0x001f) << 3;
                g2 = RGB2Luma_4bit(r, g, b);
                rgb_data = *Src;
                Src += width;
                r = (rgb_data & 0xf800) >> 8;
                g = (rgb_data & 0x07e0) >> 3;
                b = (rgb_data & 0x001f) << 3;
                g3 = RGB2Luma_4bit(r, g, b);
                *luma++ = (g0 & 0xF) | ((g1 & 0xF) << 4) | ((g2 & 0xF) << 8) | ((g3
                                & 0xF) << 12);
        }
}

void RGB565_2_Luma_180(unsigned short *luma, unsigned short *Src, unsigned int width)
{
    unsigned int i;
    unsigned int r, g, b, g0, g1, g2, g3;
    unsigned int rgb_data;

    for (i=0; i<width; i+=4)
    {
        rgb_data = *Src--;
        r = (rgb_data&0xf800)>>8;
        g = (rgb_data&0x07e0)>>3;
        b = (rgb_data&0x001f)<<3;
        g0 = RGB2Luma_4bit(r, g, b);

        rgb_data = *Src--;
        r = (rgb_data&0xf800)>>8;
        g = (rgb_data&0x07e0)>>3;
        b = (rgb_data&0x001f)<<3;
        g1 = RGB2Luma_4bit(r, g, b);

        rgb_data = *Src--;
        r = (rgb_data&0xf800)>>8;
        g = (rgb_data&0x07e0)>>3;
        b = (rgb_data&0x001f)<<3;
        g2 = RGB2Luma_4bit(r, g, b);

        rgb_data = *Src--;
        r = (rgb_data&0xf800)>>8;
        g = (rgb_data&0x07e0)>>3;
        b = (rgb_data&0x001f)<<3;
        g3 = RGB2Luma_4bit(r, g, b);
        *luma++ = (g0 & 0xF) | ((g1 & 0xF)<<4) | ((g2 & 0xF)<<8)| ((g3 & 0xF)<<12);
    }
}


void RGB565_2_Luma_270(unsigned short *luma, unsigned short *Src, unsigned int width, unsigned int height)
{
    unsigned int i;
    unsigned int r, g, b, g0, g1, g2, g3;
    unsigned int rgb_data;

    for (i=0; i<height; i+=4)
    {
        rgb_data = *Src;
        Src -= width;
        r = (rgb_data&0xf800)>>8;
        g = (rgb_data&0x07e0)>>3;
        b = (rgb_data&0x001f)<<3;
        g0 = RGB2Luma_4bit(r, g, b);

        rgb_data = *Src;
        Src -= width;
        r = (rgb_data&0xf800)>>8;
        g = (rgb_data&0x07e0)>>3;
        b = (rgb_data&0x001f)<<3;
        g1 = RGB2Luma_4bit(r, g, b);

        rgb_data = *Src;
        Src -= width;
        r = (rgb_data&0xf800)>>8;
        g = (rgb_data&0x07e0)>>3;
        b = (rgb_data&0x001f)<<3;
        g2 = RGB2Luma_4bit(r, g, b);

        rgb_data = *Src;
        Src -= width;
        r = (rgb_data&0xf800)>>8;
        g = (rgb_data&0x07e0)>>3;
        b = (rgb_data&0x001f)<<3;
        g3 = RGB2Luma_4bit(r, g, b);
        *luma++ = (g0 & 0xF) | ((g1 & 0xF)<<4) | ((g2 & 0xF)<<8)| ((g3 & 0xF)<<12);
    }
}

static void RGB565_2_Luma(unsigned short *gray, unsigned short *rgb_buf, int w, int h) {
    int i, j;
    switch(gRotate)
    {
        case 0:
            for (j=0;j<h;j++)
            {
                RGB565_2_Luma_0(gray, rgb_buf, w>>2);
                rgb_buf += w;
                gray += (w>>2);
            }
            break;
        case 1:
            rgb_buf += (w - 1);
              for(j=0; j<w; j++)
              {
                RGB565_2_Luma_90(gray, rgb_buf, w, h);
                rgb_buf -= 1;
                gray += (h>>2);
              }           
            break;
        case 2:
            rgb_buf += (h*w - 1) ;
            for(j=0; j<h; j++)
            {
                RGB565_2_Luma_180(gray, rgb_buf, w);
                rgb_buf -= w;
                gray += (w>>2);
            }
            break;
        case 3:
            rgb_buf += (h*w - w) ;
            for(j=0; j<w; j++)
            {
                RGB565_2_Luma_270(gray, rgb_buf, w, h);
                rgb_buf += 1;
                gray += (h>>2);
            }
            break;
    }
}
/////////////////////////////
#define LUMA_2_565 4

static int fb_post(struct framebuffer_device_t* dev, buffer_handle_t buffer)
{
    if (private_handle_t::validate(buffer) < 0)
        return -EINVAL;

    fb_context_t* ctx = (fb_context_t*)dev;

    private_handle_t const* hnd = reinterpret_cast<private_handle_t const*>(buffer);
    private_module_t* m = reinterpret_cast<private_module_t*>(
            dev->common.module);
    
    if (m->currentBuffer) {
        m->base.unlock(&m->base, m->currentBuffer);
        m->currentBuffer = 0;
    }

    if(gSkipPost) {
        gSkipPost = 0;
        void* temp_buffer = 0;
        m->base.lock(&m->base, buffer,
                GRALLOC_USAGE_SW_READ_RARELY,
                0, 0, m->info.xres, m->info.yres,
                &temp_buffer);

        for(int i =0;i < m->info.xres * m->info.yres;i++) {
            ((unsigned short*) temp_buffer)[i] = 0xFFFF;
        }

        m->base.unlock(&m->base, buffer);
//        return 0;
    }


    if (hnd->flags & private_handle_t::PRIV_FLAGS_FRAMEBUFFER) {
        size_t offset = hnd->base - m->framebuffer->base;
        m->info.activate = FB_ACTIVATE_VBL;
        m->info.yoffset = offset / m->finfo.line_length;

        /////////////mod by cjf/////////
        if(gRGB565_2_Luma) {
            void* buffer_vaddr;
            void* fb_buffer_vaddr;
            
            offset = hnd->base - (int)m->buffer_base;
            m->info.yoffset = offset / m->finfo.line_length / LUMA_2_565;
            m->base.lock(&m->base, buffer,
                    GRALLOC_USAGE_SW_READ_RARELY,
                    0, 0, m->info.xres, m->info.yres,
                    &buffer_vaddr);
            m->base.lock(&m->base, m->framebuffer,
                    GRALLOC_USAGE_SW_WRITE_RARELY, 
                    0, 0, m->info.xres, m->info.yres,
                    &fb_buffer_vaddr);
          
            RGB565_2_Luma((unsigned short *)(fb_buffer_vaddr + offset / LUMA_2_565), (unsigned short *)buffer_vaddr, 
                    m->info.xres, m->info.yres);
            
            m->base.unlock(&m->base, m->framebuffer);
            m->base.unlock(&m->base, buffer);
            
            m->base.lock(&m->base, buffer,
                    private_module_t::PRIV_USAGE_LOCKED_FOR_POST, 
                    0, 0, m->info.xres, m->info.yres, NULL);
            if (ioctl(m->framebuffer->fd, FBIOPUT_VSCREENINFO, &m->info) == -1) {
                LOGE("FBIOPUT_VSCREENINFO failed");
                m->base.unlock(&m->base, buffer);
                return -errno;
            }
            m->currentBuffer = buffer;
        }
        //////////////////////////////
        else {
            m->base.lock(&m->base, buffer, 
                    private_module_t::PRIV_USAGE_LOCKED_FOR_POST, 
                    0, 0, m->info.xres, m->info.yres, NULL);
            if (ioctl(m->framebuffer->fd, FBIOPUT_VSCREENINFO, &m->info) == -1) {
                LOGE("FBIOPUT_VSCREENINFO failed");
                m->base.unlock(&m->base, buffer); 
                return -errno;
            }
            m->currentBuffer = buffer;
        }
        
    } else {
        // If we can't do the page_flip, just copy the buffer to the front 
        // FIXME: use copybit HAL instead of memcpy
        
        void* fb_vaddr;
        void* buffer_vaddr;
        
        m->base.lock(&m->base, m->framebuffer, 
                GRALLOC_USAGE_SW_WRITE_RARELY, 
                0, 0, m->info.xres, m->info.yres,
                &fb_vaddr);

        m->base.lock(&m->base, buffer, 
                GRALLOC_USAGE_SW_READ_RARELY, 
                0, 0, m->info.xres, m->info.yres,
                &buffer_vaddr);
        
        /////////////mod by cjf/////////
        if(!gRGB565_2_Luma) {
            memcpy(fb_vaddr, buffer_vaddr, m->finfo.line_length * m->info.yres);
        } else {
            RGB565_2_Luma((unsigned short *)fb_vaddr, (unsigned short *)buffer_vaddr, m->info.xres, m->info.yres);
        }
        
        m->base.unlock(&m->base, buffer); 
        m->base.unlock(&m->base, m->framebuffer); 
    }
    
    return 0;
}

/*****************************************************************************/

int mapFrameBufferLocked(struct private_module_t* module)
{
    // already initialized...
    if (module->framebuffer) {
        return 0;
    }
        
    char const * const device_template[] = {
            "/dev/graphics/fb%u",
            "/dev/fb%u",
            0 };

    int fd = -1;
    int i=0;
    char name[64];

    while ((fd==-1) && device_template[i]) {
        snprintf(name, 64, device_template[i], 0);
        fd = open(name, O_RDWR, 0);
        i++;
    }
    if (fd < 0)
        return -errno;

    struct fb_fix_screeninfo finfo;
    if (ioctl(fd, FBIOGET_FSCREENINFO, &finfo) == -1)
        return -errno;

    struct fb_var_screeninfo info;
    if (ioctl(fd, FBIOGET_VSCREENINFO, &info) == -1)
        return -errno;

    info.reserved[0] = 0;
    info.reserved[1] = 0;
    info.reserved[2] = 0;
    info.xoffset = 0;
    info.yoffset = 0;
    info.activate = FB_ACTIVATE_NOW;

    /*
     * Explicitly request 5/6/5
     */
    info.bits_per_pixel = 16;
    info.red.offset     = 11;
    info.red.length     = 5;
    info.green.offset   = 5;
    info.green.length   = 6;
    info.blue.offset    = 0;
    info.blue.length    = 5;
    info.transp.offset  = 0;
    info.transp.length  = 0;


    if(gRGB565_2_Luma) {
        gRotate = info.rotate;
        char property[255];
        if (property_get("persist.sys.luma-rotate", property, NULL) > 0) {
            gRotate = atoi(property);
            LOGW("jeffy persist.sys.luma-rotate fb");
        }
        LOGW("jeffy gRotate:%d", gRotate);
        
        info.bits_per_pixel = 4;
        info.red.offset     = 0;
        info.red.length     = 4;
        info.green.offset   = 0;
        info.green.length   = 0;
        info.blue.offset    = 0;
        info.blue.length    = 0;
        info.transp.offset  = 0;
        info.transp.length  = 0;
    }
    else if(gPixel_format_l8 == 1)
    {
    info.bits_per_pixel = 8;
    info.red.offset     = 0;
    info.red.length     = 8;
    info.green.offset   = 0;
    info.green.length   = 8;
    info.blue.offset    = 0;
    info.blue.length    = 8;
    info.transp.offset  = 0;
    info.transp.length  = 0;
    }
    /*
     * Request NUM_BUFFERS screens (at lest 2 for page flipping)
     */
    info.yres_virtual = info.yres * NUM_BUFFERS;


    uint32_t flags = PAGE_FLIP;
    if (ioctl(fd, FBIOPUT_VSCREENINFO, &info) == -1) {
        info.yres_virtual = info.yres;
        flags &= ~PAGE_FLIP;
        LOGW("FBIOPUT_VSCREENINFO failed, page flipping not supported");
    }

    if (info.yres_virtual < info.yres * 2) {
        // we need at least 2 for page-flipping
        info.yres_virtual = info.yres;
        flags &= ~PAGE_FLIP;
        LOGW("page flipping not supported (yres_virtual=%d, requested=%d)",
                info.yres_virtual, info.yres*2);
    }

    if (ioctl(fd, FBIOGET_VSCREENINFO, &info) == -1)
        return -errno;

    int refreshRate = 1000000000000000LLU /
    (
            uint64_t( info.upper_margin + info.lower_margin + info.yres )
            * ( info.left_margin  + info.right_margin + info.xres )
            * info.pixclock
    );

    if (refreshRate == 0) {
        // bleagh, bad info from the driver
        refreshRate = 60*1000;  // 60 Hz
    }

    if (int(info.width) <= 0 || int(info.height) <= 0) {
        // the driver doesn't return that information
        // default to 160 dpi
        info.width  = ((info.xres * 25.4f)/160.0f + 0.5f);
        info.height = ((info.yres * 25.4f)/160.0f + 0.5f);
    }

    float xdpi = (info.xres * 25.4f) / info.width;
    float ydpi = (info.yres * 25.4f) / info.height;
    float fps  = refreshRate / 1000.0f;

    LOGI(   "using (fd=%d)\n"
            "id           = %s\n"
            "xres         = %d px\n"
            "yres         = %d px\n"
            "xres_virtual = %d px\n"
            "yres_virtual = %d px\n"
            "bpp          = %d\n"
            "r            = %2u:%u\n"
            "g            = %2u:%u\n"
            "b            = %2u:%u\n",
            fd,
            finfo.id,
            info.xres,
            info.yres,
            info.xres_virtual,
            info.yres_virtual,
            info.bits_per_pixel,
            info.red.offset, info.red.length,
            info.green.offset, info.green.length,
            info.blue.offset, info.blue.length
    );

    LOGI(   "width        = %d mm (%f dpi)\n"
            "height       = %d mm (%f dpi)\n"
            "refresh rate = %.2f Hz\n",
            info.width,  xdpi,
            info.height, ydpi,
            fps
    );


    if (ioctl(fd, FBIOGET_FSCREENINFO, &finfo) == -1)
        return -errno;

    if (finfo.smem_len <= 0)
        return -errno;


    module->flags = flags;
    module->info = info;
    module->finfo = finfo;
    module->xdpi = xdpi;
    module->ydpi = ydpi;
    module->fps = fps;

    /*
     * map the framebuffer
     */

    int err;
    size_t fbSize = roundUpToPageSize(finfo.line_length * info.yres_virtual);
    module->framebuffer = new private_handle_t(dup(fd), fbSize,
            private_handle_t::PRIV_FLAGS_USES_PMEM);

    module->numBuffers = info.yres_virtual / info.yres;
    module->bufferMask = 0;

    void* vaddr = mmap(0, fbSize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (vaddr == MAP_FAILED) {
        LOGE("Error mapping the framebuffer (%s)", strerror(errno));
        return -errno;
    }   
    module->framebuffer->base = intptr_t(vaddr);
    module->framebuffer->pbase = intptr_t(finfo.smem_start);
    /* 
     * yuzhe@rockchip modified below.
     * Clear the screen with white color instead of black color.
     */
    memset(vaddr, 0xFF, fbSize);
    return 0;
}

static int mapFrameBuffer(struct private_module_t* module)
{
    pthread_mutex_lock(&module->lock);
    int err = mapFrameBufferLocked(module);
    pthread_mutex_unlock(&module->lock);
    return err;
}

/*****************************************************************************/

static int fb_close(struct hw_device_t *dev)
{
    fb_context_t* ctx = (fb_context_t*)dev;
    
    //////add by cjf
    if(gRGB565_2_Luma) {

        private_module_t* m = (private_module_t*)ctx->device.common.module;
        if (m->buffer_base != NULL) {
            free(m->buffer_base);
            m->buffer_base = NULL;
        }
    }

    if (ctx) {
        free(ctx);
    }
    return 0;
}

int fb_device_open(hw_module_t const* module, const char* name,
        hw_device_t** device)
{
    int status = -EINVAL;
    if (!strcmp(name, GRALLOC_HARDWARE_FB0)) {
        
        alloc_device_t* gralloc_device;
        status = gralloc_open(module, &gralloc_device);
        if (status < 0)
            return status;

        /* initialize our state here */
        fb_context_t *dev = (fb_context_t*)malloc(sizeof(*dev));
        memset(dev, 0, sizeof(*dev));

        /* initialize the procs */
        dev->device.common.tag = HARDWARE_DEVICE_TAG;
        dev->device.common.version = 0;
        dev->device.common.module = const_cast<hw_module_t*>(module);
        dev->device.common.close = fb_close;
        dev->device.setSwapInterval = fb_setSwapInterval;
        dev->device.post            = fb_post;
        dev->device.setUpdateRect = 0;

        private_module_t* m = (private_module_t*)module;


        /////////////add by cjf/////////
        char property[255];
        if (property_get("ro.rgb565-2-luma", property, NULL) > 0) {
            if (atoi(property) == 1) {
                LOGW("jeffy ro.rgb565-2-luma fb");
                gRGB565_2_Luma = 1;
            }
        }
        if(!gRGB565_2_Luma) {
            if (property_get("ro.pixel_format_l_8", property, NULL) > 0) 
            {//hjk
                if (atoi(property) == 1)
                {
                    gPixel_format_l8 = 1;
                }
            }
        }
        
        status = mapFrameBuffer(m);
        if (status >= 0) {
            int stride = m->finfo.line_length / m->info.bits_per_pixel >> 3;
            const_cast<uint32_t&>(dev->device.flags) = 0;
            const_cast<uint32_t&>(dev->device.width) = m->info.xres;
            const_cast<uint32_t&>(dev->device.height) = m->info.yres;
            const_cast<int&>(dev->device.stride) = stride;
            const_cast<int&>(dev->device.format) = HAL_PIXEL_FORMAT_RGB_565;
            if(gPixel_format_l8)
            {
                const_cast<int&>(dev->device.format) = HAL_PIXEL_FORMAT_L_8;
            }
            const_cast<float&>(dev->device.xdpi) = m->xdpi;
            const_cast<float&>(dev->device.ydpi) = m->ydpi;
            const_cast<float&>(dev->device.fps) = m->fps;
            const_cast<int&>(dev->device.minSwapInterval) = 1;
            const_cast<int&>(dev->device.maxSwapInterval) = 1;
            *device = &dev->device.common;
            
            if(gRGB565_2_Luma) {
                const size_t bufferSize = m->finfo.line_length * m->info.yres * LUMA_2_565;
                if (m->buffer_base == NULL) {
                    m->buffer_base = malloc(2 * bufferSize);
                    if (m->buffer_base == NULL) {
                        return -1;
                    }
                }
            }
        }
    }

    return status;
}
