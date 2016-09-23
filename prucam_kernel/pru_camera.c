//
//  pru_camera.c
//
//
//  Created by Chao Jiang on 08/07/2016.
//
//


#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/interrupt.h>

/* pruss */
#include <linux/pruss.h>
#include <../drivers/remoteproc/pruss.h>

/* v4l2 */
#include <linux/videodev2.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-dma-contig.h>

/* sensors */
#include <media/ov7670.h>

#include "pru_camera.h"
#include "pru_camera_reg.h"

#define sensor_call(cam, o, f, args...) v4l2_subdev_call(cam->sensor, o, f, ##args)

static struct prucam_format_struct {
	__u8 *desc;
	__u32 pixelformat;
	int bpp;   /* Bytes per pixel */
	bool planar;
	u32 mbus_code;
} prucam_formats[] = {
	{
		.desc		= "YUYV 4:2:2",
		.pixelformat	= V4L2_PIX_FMT_YUYV,
		.mbus_code	= MEDIA_BUS_FMT_YUYV8_2X8,
		.bpp		= 2,
		.planar		= false,
	},
	{
		.desc		= "YVYU 4:2:2",
		.pixelformat	= V4L2_PIX_FMT_YVYU,
		.mbus_code	= MEDIA_BUS_FMT_YUYV8_2X8,
		.bpp		= 2,
		.planar		= false,
	},
	{
		.desc		= "YUV 4:2:2 PLANAR",
		.pixelformat	= V4L2_PIX_FMT_YUV422P,
		.mbus_code	= MEDIA_BUS_FMT_YUYV8_2X8,
		.bpp		= 2,
		.planar		= true,
	},
	{
		.desc		= "YUV 4:2:0 PLANAR",
		.pixelformat	= V4L2_PIX_FMT_YUV420,
		.mbus_code	= MEDIA_BUS_FMT_YUYV8_2X8,
		.bpp		= 2,
		.planar		= true,
	},
	{
		.desc		= "YVU 4:2:0 PLANAR",
		.pixelformat	= V4L2_PIX_FMT_YVU420,
		.mbus_code	= MEDIA_BUS_FMT_YUYV8_2X8,
		.bpp		= 2,
		.planar		= true,
	},
	{
		.desc		= "RGB 444",
		.pixelformat	= V4L2_PIX_FMT_RGB444,
		.mbus_code	= MEDIA_BUS_FMT_RGB444_2X8_PADHI_LE,
		.bpp		= 2,
		.planar		= false,
	},
	{
		.desc		= "RGB 565",
		.pixelformat	= V4L2_PIX_FMT_RGB565,
		.mbus_code	= MEDIA_BUS_FMT_RGB565_2X8_LE,
		.bpp		= 2,
		.planar		= false,
	},
	{
		.desc		= "Raw RGB Bayer",
		.pixelformat	= V4L2_PIX_FMT_SBGGR8,
		.mbus_code	= MEDIA_BUS_FMT_SBGGR8_1X8,
		.bpp		= 1,
		.planar		= false,
	},
};
#define N_PRUCAM_FMTS ARRAY_SIZE(prucam_formats)

static struct prucam_format_struct *prucam_find_format(u32 pixelformat) {
    unsigned i;

    for (i = 0; i < N_PRUCAM_FMTS; i++)
        if (prucam_formats[i].pixelformat == pixelformat)
            return prucam_formats + i;
    /* Not found? Then return the first format. */
    return prucam_formats;
}

static const struct v4l2_pix_format prucam_def_pix_format = {
	.width          = VGA_WIDTH,
	.height         = VGA_HEIGHT,
	.pixelformat    = V4L2_PIX_FMT_YUYV,
	.field          = V4L2_FIELD_NONE,
	.bytesperline   = VGA_WIDTH * 2,
	.sizeimage      = VGA_WIDTH * VGA_HEIGHT * 2,
};

static const u32 prucam_def_mbus_code = MEDIA_BUS_FMT_YUYV8_2X8;

static void prucam_reset(struct pru_camera *cam) { //consider doing soft reset in pru as well
	unsigned long flags;

    spin_lock_irqsave(&cam->dev_lock, flags);
    gpio_set_value(cam->rst_pin, 0);
    spin_unlock_irqrestore(&cam->dev_lock, flags);
    msleep(10);
    spin_lock_irqsave(&cam->dev_lock, flags);
    gpio_set_value(cam->rst_pin, 1);
    spin_unlock_irqrestore(&cam->dev_lock, flags);
}

static void prucam_ctrl_power_up(struct pru_camera *cam) {
	unsigned long flags;

    spin_lock_irqsave(&cam->dev_lock, flags);
    gpio_set_value(cam->pwdn_pin, 0);
    spin_unlock_irqrestore(&cam->dev_lock, flags);
}

static void prucam_ctrl_power_down(struct pru_camera *cam) {
	unsigned long flags;

    spin_lock_irqsave(&cam->dev_lock, flags);
    gpio_set_value(cam->pwdn_pin, 1);
    spin_unlock_irqrestore(&cam->dev_lock, flags);
}

static int prucam_ctrl_start(struct pru_camera *cam) {
	return prucam_reg_write(REG_CTRL, 1);
}

static int prucam_ctrl_stop(struct pru_camera *cam) {
    return prucam_reg_write(REG_CTRL, 0);
}

static void prucam_buffer_done(struct pru_camera *cam, struct vb2_buffer *vbuf, int frame) {
	vbuf->v4l2_buf.bytesused = cam->pix_format.sizeimage;
	vbuf->v4l2_buf.sequence = frame;    //TODO check this
	vb2_set_plane_payload(vbuf, 0, cam->pix_format.sizeimage);
	vb2_buffer_done(vbuf, VB2_BUF_STATE_DONE);
}

/* ----------------------------------------------------------------------- */
/*
 * Videobuf2 interface code.
 */

static int prucam_vb_queue_setup(struct vb2_queue *vq,
                                 const struct v4l2_format *fmt,
                                 unsigned int *num_buffers,
                                 unsigned int *num_planes,
                                 unsigned int sizes[],
                                 void *alloc_ctxs[]) {
    struct pru_camera *cam = vb2_get_drv_priv(vq);
    int minbufs = 4;

    sizes[0] = cam->pix_format.sizeimage;
    *num_planes = 1; /* Someday we have to support planar formats... */
    if (*num_buffers < minbufs)
        *num_buffers = minbufs;
    alloc_ctxs[0] = cam->vb_alloc_ctx;

    return 0;
}


static void prucam_vb_buf_queue(struct vb2_buffer *vb)
{
    struct prucam_vb_buffer *pvb = vb_to_pvb(vb);
    struct pru_camera *cam = vb2_get_drv_priv(vb->vb2_queue);
    unsigned long flags;

    spin_lock_irqsave(&cam->dev_lock, flags);
    list_add(&pvb->queue, &cam->buffers);

	spin_unlock_irqrestore(&cam->dev_lock, flags);
}

/*
 * These need to be called with the mutex held from vb2
 */
static int prucam_vb_start_streaming(struct vb2_queue *vq, unsigned int count)
{
    unsigned long flags;
    struct pru_camera *cam = vb2_get_drv_priv(vq);
	struct prucam_vb_buffer *pvb;

	if (cam->state != S_IDLE) {
		INIT_LIST_HEAD(&cam->buffers);
		return -EINVAL;
	}

	cam->sequence = 0; //re-init sequence number

    //check buffers
    if (list_empty(&cam->buffers))
        return -EINVAL;             // no buffer available, hence invalid operation
    else {							// have buffers, so get the first to pru and remove it for queue
        pvb = list_first_entry(&cam->buffers, struct prucam_vb_buffer, queue);
        list_del_init(&pvb->queue);
        prucam_buf_queue(pvb->dma_desc_pa);
        cam->vb_bufs[0] = pvb;

        if (list_empty(&cam->buffers)) {	//run it the second time to queue send one additional to pru if available
            pvb = list_first_entry(&cam->buffers, struct prucam_vb_buffer, queue);
            list_del_init(&pvb->queue);
            prucam_buf_queue(pvb->dma_desc_pa);
            cam->vb_bufs[1] = pvb;
        }
	}

    //enable
    spin_lock_irqsave(&cam->dev_lock, flags);
    prucam_ctrl_start(cam);  //let pru start grabbing
    spin_unlock_irqrestore(&cam->dev_lock, flags);

    return 0;
}

static void prucam_vb_stop_streaming(struct vb2_queue *vq)
{
    struct pru_camera *cam = vb2_get_drv_priv(vq);
    unsigned long flags;

    if (cam->state != S_STREAMING)
	    return;

    spin_lock_irqsave(&cam->dev_lock, flags);
    prucam_ctrl_stop(cam);
    spin_unlock_irqrestore(&cam->dev_lock, flags);

    /*
     * VB2 reclaims the buffers, so we need to forget
     * about them.
     */
    spin_lock_irqsave(&cam->dev_lock, flags);
    INIT_LIST_HEAD(&cam->buffers);
    spin_unlock_irqrestore(&cam->dev_lock, flags);
}

static const struct vb2_ops prucam_vb2_ops = {
    .queue_setup        = prucam_vb_queue_setup,
    .buf_queue          = prucam_vb_buf_queue,
    .start_streaming    = prucam_vb_start_streaming,
    .stop_streaming     = prucam_vb_stop_streaming,
    .wait_prepare       = vb2_ops_wait_prepare,
    .wait_finish        = vb2_ops_wait_finish,
};

static int prucam_setup_vb2(struct pru_camera *cam) {
    struct vb2_queue *vq = &cam->vb_queue;

    memset(vq, 0, sizeof(*vq));
    vq->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    vq->drv_priv = cam;
    vq->lock = &cam->s_mutex;
    INIT_LIST_HEAD(&cam->buffers);

    vq->ops = &prucam_vb2_ops;
    vq->mem_ops = &vb2_dma_contig_memops;
    vq->buf_struct_size = sizeof(struct prucam_vb_buffer);
    vq->io_modes = VB2_MMAP;
	vq->min_buffers_needed = 3;
	vq->timestamp_flags = V4L2_BUF_FLAG_TSTAMP_SRC_SOE | V4L2_BUF_FLAG_TIMESTAMP_COPY;

    cam->vb_alloc_ctx = vb2_dma_contig_init_ctx(cam->dev);
    if (IS_ERR(cam->vb_alloc_ctx))
        return PTR_ERR(cam->vb_alloc_ctx);

    return vb2_queue_init(vq);
}

static void prucam_cleanup_vb2(struct pru_camera *cam) {
    vb2_queue_release(&cam->vb_queue);
    vb2_dma_contig_cleanup_ctx(cam->vb_alloc_ctx);
}

/* ---------------------------------------------------------------------- */
/*
 * The long list of V4L2 ioctl() operations.
 */

static int prucam_vidioc_streamon(struct file *filp, void *priv, enum v4l2_buf_type type) {
    struct pru_camera *cam = filp->private_data;
    int ret;

    mutex_lock(&cam->s_mutex);
    ret = vb2_streamon(&cam->vb_queue, type);
    mutex_unlock(&cam->s_mutex);
    return ret;
}


static int prucam_vidioc_streamoff(struct file *filp, void *priv, enum v4l2_buf_type type) {
    struct pru_camera *cam = filp->private_data;
    int ret;

    mutex_lock(&cam->s_mutex);
    ret = vb2_streamoff(&cam->vb_queue, type);
    mutex_unlock(&cam->s_mutex);
    return ret;
}


static int prucam_vidioc_reqbufs(struct file *filp, void *priv, struct v4l2_requestbuffers *req) {
    struct pru_camera *cam = filp->private_data;
    int ret;

    mutex_lock(&cam->s_mutex);
    ret = vb2_reqbufs(&cam->vb_queue, req);
    mutex_unlock(&cam->s_mutex);
    return ret;
}


static int prucam_vidioc_querybuf(struct file *filp, void *priv, struct v4l2_buffer *buf) {
    struct pru_camera *cam = filp->private_data;
    int ret;

    mutex_lock(&cam->s_mutex);
    ret = vb2_querybuf(&cam->vb_queue, buf);
    mutex_unlock(&cam->s_mutex);
    return ret;
}

static int prucam_vidioc_qbuf(struct file *filp, void *priv, struct v4l2_buffer *buf) {
    struct pru_camera *cam = filp->private_data;
    int ret;

    mutex_lock(&cam->s_mutex);
    ret = vb2_qbuf(&cam->vb_queue, buf);
    mutex_unlock(&cam->s_mutex);
    return ret;
}

static int prucam_vidioc_dqbuf(struct file *filp, void *priv, struct v4l2_buffer *buf) {
    struct pru_camera *cam = filp->private_data;
    int ret;

    mutex_lock(&cam->s_mutex);
    ret = vb2_dqbuf(&cam->vb_queue, buf, filp->f_flags & O_NONBLOCK);
    mutex_unlock(&cam->s_mutex);
    return ret;
}

static int prucam_vidioc_querycap(struct file *file, void *priv, struct v4l2_capability *cap) {
    strcpy(cap->driver, "pru_camera");
    strcpy(cap->card, "pru_camera");
    cap->device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_READWRITE | V4L2_CAP_STREAMING;
    cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS;
    return 0;
}


static int prucam_vidioc_enum_fmt_vid_cap(struct file *filp, void *priv, struct v4l2_fmtdesc *fmt) {
    if (fmt->index >= N_PRUCAM_FMTS)
        return -EINVAL;
    strlcpy(fmt->description, prucam_formats[fmt->index].desc,
            sizeof(fmt->description));
    fmt->pixelformat = prucam_formats[fmt->index].pixelformat;
    return 0;
}

static int prucam_vidioc_try_fmt_vid_cap(struct file *filp, void *priv, struct v4l2_format *fmt) {
    struct pru_camera *cam = priv;
    struct prucam_format_struct *f;
    struct v4l2_pix_format *pix = &fmt->fmt.pix;
    struct v4l2_mbus_framefmt mbus_fmt;
    int ret;

    f = prucam_find_format(pix->pixelformat);
    pix->pixelformat = f->pixelformat;
    v4l2_fill_mbus_format(&mbus_fmt, pix, f->mbus_code);
    mutex_lock(&cam->s_mutex);
    ret = sensor_call(cam, video, try_mbus_fmt, &mbus_fmt);
    mutex_unlock(&cam->s_mutex);
    v4l2_fill_pix_format(pix, &mbus_fmt);
    switch (f->pixelformat) {
        case V4L2_PIX_FMT_YUV420:
        case V4L2_PIX_FMT_YVU420:
            pix->bytesperline = pix->width * 3 / 2;
            break;
        default:
            pix->bytesperline = pix->width * f->bpp;
            break;
    }
    pix->sizeimage = pix->height * pix->bytesperline;
    return ret;
}

static int prucam_vidioc_s_fmt_vid_cap(struct file *filp, void *priv, struct v4l2_format *fmt) {
    struct pru_camera *cam = priv;
    struct prucam_format_struct *f;
    int ret;

    /*
     * Can't do anything if the device is not idle
     * Also can't if there are streaming buffers in place.
     */
    if (cam->state != S_IDLE || cam->vb_queue.num_buffers > 0) //this might not be needed as there's no state
        return -EBUSY;

    f = prucam_find_format(fmt->fmt.pix.pixelformat);

    /*
     * See if the formatting works in principle.
     */
    ret = prucam_vidioc_try_fmt_vid_cap(filp, priv, fmt);
    if (ret)
        return ret;
    /*
     * Now we start to change things for real, so let's do it
     * under lock. TODO: need check the following fro buffers, maybe
     */
    mutex_lock(&cam->s_mutex);
    cam->pix_format = fmt->fmt.pix;
    cam->mbus_code = f->mbus_code;

	//write changes to pru as well
	prucam_reg_write(REG_HEIGHT, cam->pix_format.height);
	prucam_reg_write(REG_WIDTH, cam->pix_format.width);

    mutex_unlock(&cam->s_mutex);
    return ret;
}

/*
 * Return our stored notion of how the camera is/should be configured.
 * The V4l2 spec wants us to be smarter, and actually get this from
 * the camera (and not mess with it at open time).  Someday.
 */
static int prucam_vidioc_g_fmt_vid_cap(struct file *filp, void *priv, struct v4l2_format *f) {
    struct pru_camera *cam = priv;

    f->fmt.pix = cam->pix_format;
    return 0;
}

/*
 * We only have one input - the sensor - so minimize the nonsense here.
 */
static int prucam_vidioc_enum_input(struct file *filp, void *priv, struct v4l2_input *input) {
    if (input->index != 0)
        return -EINVAL;

    input->type = V4L2_INPUT_TYPE_CAMERA;
    input->std = V4L2_STD_ALL; /* Not sure what should go here */
    strcpy(input->name, "Camera");
    return 0;
}

static int prucam_vidioc_g_input(struct file *filp, void *priv, unsigned int *i) {
    *i = 0;
    return 0;
}

static int prucam_vidioc_s_input(struct file *filp, void *priv, unsigned int i) {
    if (i != 0)
        return -EINVAL;
    return 0;
}

/* from vivi.c */
static int prucam_vidioc_s_std(struct file *filp, void *priv, v4l2_std_id a) {
    return 0;
}

static int prucam_vidioc_g_std(struct file *filp, void *priv, v4l2_std_id *a) {
    *a = V4L2_STD_NTSC_M;
    return 0;
}

/*
 * G/S_PARM.  Most of this is done by the sensor, but we are
 * the level which controls the number of read buffers.
 */
static int prucam_vidioc_g_parm(struct file *filp, void *priv, struct v4l2_streamparm *parms) {
    struct pru_camera *cam = priv;
    int ret;

    mutex_lock(&cam->s_mutex);
    ret = sensor_call(cam, video, g_parm, parms);
    mutex_unlock(&cam->s_mutex);
    parms->parm.capture.readbuffers = 3;    //make this variable?
    return ret;
}

static int prucam_vidioc_s_parm(struct file *filp, void *priv, struct v4l2_streamparm *parms) {
    struct pru_camera *cam = priv;
    int ret;

    mutex_lock(&cam->s_mutex);
    ret = sensor_call(cam, video, s_parm, parms);
    mutex_unlock(&cam->s_mutex);
    parms->parm.capture.readbuffers = 3;    //make this variable?
    return ret;
}

static int prucam_vidioc_enum_framesizes(struct file *filp, void *priv, struct v4l2_frmsizeenum *sizes) {
    struct pru_camera *cam = priv;
    struct prucam_format_struct *f;
    struct v4l2_subdev_frame_size_enum fse = {
        .index = sizes->index,
        .which = V4L2_SUBDEV_FORMAT_ACTIVE,
    };
    int ret;

    f = prucam_find_format(sizes->pixel_format);
    if (f->pixelformat != sizes->pixel_format)
        return -EINVAL;
    fse.code = f->mbus_code;
    mutex_lock(&cam->s_mutex);
    ret = sensor_call(cam, pad, enum_frame_size, NULL, &fse);
    mutex_unlock(&cam->s_mutex);
    if (ret)
        return ret;
    if (fse.min_width == fse.max_width &&
        fse.min_height == fse.max_height) {
        sizes->type = V4L2_FRMSIZE_TYPE_DISCRETE;
        sizes->discrete.width = fse.min_width;
        sizes->discrete.height = fse.min_height;
        return 0;
    }
    sizes->type = V4L2_FRMSIZE_TYPE_CONTINUOUS;
    sizes->stepwise.min_width = fse.min_width;
    sizes->stepwise.max_width = fse.max_width;
    sizes->stepwise.min_height = fse.min_height;
    sizes->stepwise.max_height = fse.max_height;
    sizes->stepwise.step_width = 1;
    sizes->stepwise.step_height = 1;
    return 0;
}

static int prucam_vidioc_enum_frameintervals(struct file *filp, void *priv, struct v4l2_frmivalenum *interval) {
    struct pru_camera *cam = priv;
    struct prucam_format_struct *f;
    struct v4l2_subdev_frame_interval_enum fie = {
        .index = interval->index,
        .width = interval->width,
        .height = interval->height,
        .which = V4L2_SUBDEV_FORMAT_ACTIVE,
    };
    int ret;

    f = prucam_find_format(interval->pixel_format);
    if (f->pixelformat != interval->pixel_format)
        return -EINVAL;
    fie.code = f->mbus_code;
    mutex_lock(&cam->s_mutex);
    ret = sensor_call(cam, pad, enum_frame_interval, NULL, &fie);
    mutex_unlock(&cam->s_mutex);
    if (ret)
        return ret;
    interval->type = V4L2_FRMIVAL_TYPE_DISCRETE;
    interval->discrete = fie.interval;
    return 0;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG
static int prucam_vidioc_g_register(struct file *file, void *priv, struct v4l2_dbg_register *reg) {
    struct pru_camera *cam = priv;
	int ret;

    if (reg->reg > cam->regs_size - 4)
        return -EINVAL;
    ret = prucam_reg_read((unsigned int)reg->reg, (unsigned int *)&reg->val);
    reg->size = 4;
    return ret;
}

static int prucam_vidioc_s_register(struct file *file, void *priv, const struct v4l2_dbg_register *reg) {
    struct pru_camera *cam = priv;
	int ret;

    if (reg->reg > cam->regs_size - 4)
        return -EINVAL;
    ret = prucam_reg_write((unsigned int)reg->reg, (unsigned int)reg->val);
    return ret;
}
#endif

static const struct v4l2_ioctl_ops prucam_v4l_ioctl_ops = {
    .vidioc_querycap            = prucam_vidioc_querycap,
    .vidioc_enum_fmt_vid_cap    = prucam_vidioc_enum_fmt_vid_cap,
    .vidioc_try_fmt_vid_cap     = prucam_vidioc_try_fmt_vid_cap,
    .vidioc_s_fmt_vid_cap       = prucam_vidioc_s_fmt_vid_cap,
    .vidioc_g_fmt_vid_cap       = prucam_vidioc_g_fmt_vid_cap,
    .vidioc_enum_input          = prucam_vidioc_enum_input,
    .vidioc_g_input             = prucam_vidioc_g_input,
    .vidioc_s_input             = prucam_vidioc_s_input,
    .vidioc_s_std               = prucam_vidioc_s_std,
    .vidioc_g_std               = prucam_vidioc_g_std,
    .vidioc_reqbufs             = prucam_vidioc_reqbufs,
    .vidioc_querybuf            = prucam_vidioc_querybuf,
    .vidioc_qbuf                = prucam_vidioc_qbuf,
    .vidioc_dqbuf               = prucam_vidioc_dqbuf,
    .vidioc_streamon            = prucam_vidioc_streamon,
    .vidioc_streamoff           = prucam_vidioc_streamoff,
    .vidioc_g_parm              = prucam_vidioc_g_parm,
    .vidioc_s_parm              = prucam_vidioc_s_parm,
    .vidioc_enum_framesizes     = prucam_vidioc_enum_framesizes,
    .vidioc_enum_frameintervals = prucam_vidioc_enum_frameintervals,
#ifdef CONFIG_VIDEO_ADV_DEBUG
    .vidioc_g_register          = prucam_vidioc_g_register,
    .vidioc_s_register          = prucam_vidioc_s_register,
#endif
};

/* ---------------------------------------------------------------------- */
/*
 * Our various file operations.
 */
static int prucam_v4l_open(struct file *filp) {
    struct pru_camera *cam = video_drvdata(filp);
    int ret = 0;

    filp->private_data = cam;

//    cam->frame_state.frames = 0;
//    cam->frame_state.singles = 0;
//    cam->frame_state.delivered = 0;
    mutex_lock(&cam->s_mutex);
    if (cam->users == 0) {
        ret = prucam_setup_vb2(cam);
        if (ret)
            goto out;
        prucam_ctrl_power_up(cam);
        prucam_reset(cam);
    }
    (cam->users)++;
out:
    mutex_unlock(&cam->s_mutex);
    return ret;
}


static int prucam_v4l_release(struct file *filp) {
    struct pru_camera *cam = filp->private_data;

    dev_dbg(cam->dev, "Releasing pru camera\n");
    mutex_lock(&cam->s_mutex);
    (cam->users)--;
    if (cam->users == 0) {
        prucam_cleanup_vb2(cam);
        prucam_ctrl_power_down(cam);
    }

    mutex_unlock(&cam->s_mutex);
    return 0;
}

static ssize_t prucam_v4l_read(struct file *filp,
                             char __user *buffer,
                             size_t len,
                             loff_t *pos) {
    struct pru_camera *cam = filp->private_data;
    int ret;

    mutex_lock(&cam->s_mutex);
    ret = vb2_read(&cam->vb_queue, buffer, len, pos,
                   filp->f_flags & O_NONBLOCK);
    mutex_unlock(&cam->s_mutex);
    return ret;
}



static unsigned int prucam_v4l_poll(struct file *filp,
                                  struct poll_table_struct *pt) {
    struct pru_camera *cam = filp->private_data;
    int ret;

    mutex_lock(&cam->s_mutex);
    ret = vb2_poll(&cam->vb_queue, filp, pt);
    mutex_unlock(&cam->s_mutex);
    return ret;
}


static int prucam_v4l_mmap(struct file *filp, struct vm_area_struct *vma) {
    struct pru_camera *cam = filp->private_data;
    int ret;

    mutex_lock(&cam->s_mutex);
    ret = vb2_mmap(&cam->vb_queue, vma);
    mutex_unlock(&cam->s_mutex);
    return ret;
}

static const struct v4l2_file_operations prucam_v4l_fops = {
    .owner = THIS_MODULE,
    .open = prucam_v4l_open,
    .release = prucam_v4l_release,
    .read = prucam_v4l_read,
    .poll = prucam_v4l_poll,
    .mmap = prucam_v4l_mmap,
    .unlocked_ioctl = video_ioctl2,
};

static const struct video_device prucam_v4l_template = {
	.name = "prucam",
	.tvnorms = V4L2_STD_NTSC_M,

	.fops = &prucam_v4l_fops,
	.ioctl_ops = &prucam_v4l_ioctl_ops,
	.release = video_device_release_empty,
};

static int of_pru_camera_get_pins(struct device_node *np, unsigned int *rst_pin, unsigned int *pwdn_pin) {
	*rst_pin = of_get_named_gpio(np, "rst-pin", 0);
	*pwdn_pin = of_get_named_gpio(np, "pwdn-pin", 0);

	if (!gpio_is_valid(*rst_pin) || !gpio_is_valid(*pwdn_pin)) {
		pr_err("%s: invalid GPIO pins, rst=%d/pwdn=%d\n",
				np->full_name, *rst_pin, *pwdn_pin);
		return -ENODEV;
	}

	return 0;
}

static const struct of_device_id sensor_of_match[];
static int of_pru_camera_get_sensor_bus(struct device_node *np, enum sensor_bus_id *bus_id, void **adap, const void **data) {
    struct device_node  *child;
    struct device_node  *bus_node;
    const struct of_device_id *match;
    int ret;

    for_each_available_child_of_node(np, child) {
        match = of_match_node(sensor_of_match, child);
        if (match)  //find the first match, should only contain one match
            break;
    }
    if (!match) {
        pr_err("%s: cannot find sensor entry in DT!\n", np->full_name);
        ret = -ENODEV;
        goto exit;
    }

	*data = match->data;
    bus_node = of_parse_phandle(child, "bus", 0);
    if (bus_node == NULL) {
        ret = -ENODEV;
        goto put_child;
    }

    if (!strcmp(bus_node->name, "i2c")) {
	    *bus_id = I2C;
        *adap = of_find_i2c_adapter_by_node(bus_node);
    	if (*adap == NULL) {
        	pr_err("%s: i2c bus doesn't exist, check DT entry!\n", np->full_name);
			ret = -ENODEV;
			goto put_bus;
    	}
	    put_device(&(*(struct i2c_adapter **)adap)->dev); //decrease ref no straight, should this be done after register v4l-subdev?
    }
    else{
	    *adap = NULL;
	    *data = NULL;
        ret = -ENODEV;
        pr_err("%s: unknown bus in DT entry, only i2c is supported for now\n", np->full_name);
        goto put_bus;
    }

    ret = 0;

put_bus:
    of_node_put(bus_node);
put_child:
    of_node_put(child);
exit:
    return ret;
}

//interrupt call back
static irqreturn_t prucam_irq(int irq, void *data) {
	struct pru_camera *cam = data;
	struct prucam_vb_buffer *pvb;

	cam->sequence ++;
	pr_info("serving prucam_irq, sequence# %d\n", cam->sequence);

	pvb = cam->vb_bufs[0];
	prucam_buffer_done(cam, &pvb->vb_buf, cam->sequence);
	cam->vb_bufs[0] = 0;
	if (cam->vb_bufs[1]) {
		cam->vb_bufs[0] = cam->vb_bufs[1];
		cam->vb_bufs[1] = 0;
	}

	if (!list_empty(&cam->buffers)) {			//still have buffers available in queue, used it for next frame
		pvb = list_first_entry(&cam->buffers, struct prucam_vb_buffer, queue);
		list_del_init(&pvb->queue);
		prucam_buf_queue(pvb->dma_desc_pa);
		if (cam->vb_bufs[0])
			cam->vb_bufs[1] = pvb;
		else
			cam->vb_bufs[0] = pvb;
	}

	return IRQ_RETVAL(1);
}

static int pru_cam_probe(struct platform_device *pdev) {

    struct pru_camera *cam;
    struct device *pruss_dev;
	struct platform_device *pruss_pdev;
    unsigned int rst_pin, pwdn_pin;
	unsigned int irq;
    void *adap;
    const void *subdev_data;
    enum sensor_bus_id bus_id;
    int ret;

    dev_info(&pdev->dev, "probing pru-camera ...\n");

	//get info from DT
    ret = of_pru_camera_get_pins(pdev->dev.of_node, &rst_pin, &pwdn_pin);
    if (ret)
	    return ret;

	ret = devm_gpio_request(&pdev->dev, rst_pin, "rst");
	if (ret)
		return ret;
	ret = devm_gpio_request(&pdev->dev, pwdn_pin, "pwdn");
	if (ret)
		return ret;

	ret = of_pru_camera_get_sensor_bus(pdev->dev.of_node, &bus_id, &adap, &subdev_data);
	if (ret)
		return ret;

    //init device structure
    cam = devm_kzalloc(&pdev->dev, sizeof(struct pru_camera), GFP_KERNEL);
    if (!cam)
        return -ENOMEM;

    cam->dev = &pdev->dev;
    cam->rst_pin = rst_pin;
	cam->pwdn_pin = pwdn_pin;

    //check if pru status
    pruss_dev = bus_find_device_by_name(&platform_bus_type, NULL, "4a300000.pruss");
    if (!pruss_dev)
        return -ENODEV;
	pruss_pdev = container_of(pruss_dev, struct platform_device, dev);
	irq = platform_get_irq(pruss_pdev, 0);
	dev_info(cam->dev, "irq got from pruss %s: %d\n", pruss_pdev->name, irq);

    //need some more thorough check here to enusure it is the correct pru code running on both of the cores

    put_device(pruss_dev);
    dev_info(cam->dev, "pruss checked\n");

	ret = request_irq(irq, prucam_irq, IRQF_SHARED, "prucam-irq", cam);
	if (ret)
		return ret;

	dev_info(cam->dev, "irq requested\n");

    //v4l-dev init
    ret = v4l2_device_register(&pdev->dev, &cam->v4l2_dev);
    if (ret)
    	return ret;
	dev_info(cam->dev, "v4l2 device registered\n");

	mutex_init(&cam->s_mutex);
	cam->regs_size = 4;
	cam->state = S_NOTREADY;
	cam->pix_format = prucam_def_pix_format;
	cam->mbus_code = prucam_def_mbus_code;
    INIT_LIST_HEAD(&cam->buffers);
	dev_info(cam->dev, "cam structure initialised\n");

	//init pru regs
	prucam_reg_write(REG_HEIGHT, cam->pix_format.height);
	prucam_reg_write(REG_WIDTH, cam->pix_format.width);

	dev_info(cam->dev, "device initialised\n");

    //v4l-subdev init
    switch (bus_id) {
	    case I2C:
    		cam->sensor = v4l2_i2c_new_subdev_board(&cam->v4l2_dev, (struct i2c_adapter *)adap, (struct i2c_board_info *)subdev_data, NULL);
    		if (cam->sensor == NULL) {
	    		dev_err(cam->dev, "failed to register i2c subdev\n");
        		ret = -ENODEV;
        		goto err_return1;
    		}
    		break;
    }

	//dubious ctrl handle setup
	ret = v4l2_ctrl_handler_init(&cam->ctrl_handler, 10);
	if (ret)
		goto err_return1;
	cam->v4l2_dev.ctrl_handler = &cam->ctrl_handler;
	dev_info(cam->dev, "control handle initialised\n");

    //register video device
    mutex_lock(&cam->s_mutex);
    cam->vdev = prucam_v4l_template;
    cam->vdev.v4l2_dev = &cam->v4l2_dev;
    video_set_drvdata(&cam->vdev, cam);
    ret = video_register_device(&cam->vdev, VFL_TYPE_GRABBER, -1);
    if (ret)
        goto err_return2;
	dev_info(cam->dev, "video device registered\n");

	//TODO buffer allocation here?

err_return2:
    v4l2_ctrl_handler_free(&cam->ctrl_handler);
    mutex_unlock(&cam->s_mutex);
	dev_info(cam->dev, "probing finished\n");
    return ret;

err_return1:
    v4l2_device_unregister(&cam->v4l2_dev);
    return ret;
}

static int pru_cam_remove(struct platform_device *pdev) {
    struct v4l2_device *v4l2_dev = platform_get_drvdata(pdev);

    v4l2_device_unregister(v4l2_dev);

    dev_info(&pdev->dev, "pru-camera removed\n");

    return 0;
}

#if defined(CONFIG_OF)
static struct ov7670_config ov7670_dfl_cfg = {
    .min_width = 320,
    .min_height = 240,
    .clock_speed = 25,
    .use_smbus = false,
    .pll_bypass = false,
    .pclk_hb_disable = false,
};

static struct i2c_board_info ov7670_info = {
    .type = "ov7670",
    .addr = 0x21,
    .flags = I2C_CLIENT_SCCB,
    .platform_data = &ov7670_dfl_cfg,
};

static const struct of_device_id sensor_of_match[] = {
    {.compatible = "ovti,ov7670", .data = &ov7670_info},
    {},
};

static const struct of_device_id pru_camera_dt_ids[] = {
    {.compatible = "pru-camera",},
    {}
};

MODULE_DEVICE_TABLE(of, pru_camera_dt_ids);
#endif

static struct platform_driver pru_camera_driver = {
    .driver             = {
        .name           = "pru-camera",
        .of_match_table = of_match_ptr(pru_camera_dt_ids),
    },
    .probe              = pru_cam_probe,
    .remove             = pru_cam_remove,
};

module_platform_driver(pru_camera_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Chao Jiang");
MODULE_DESCRIPTION("PRU interfaced Camera driver");
