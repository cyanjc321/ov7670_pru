//
//  pru_camera.h
//
//
//  Created by Chao Jiang on 08/07/2016.
//
//

#ifndef pru_camera_h
#define pru_camera_h

#define MAX_BUF_NUMBER		2

enum prucam_state {
	S_NOTREADY,
	S_IDLE,
	S_STREAMING,
	S_FLAKED
};

enum sensor_bus_id {
	I2C
};

struct prucam_vb_buffer {
	struct vb2_buffer           vb_buf;
	struct list_head            queue;
	dma_addr_t                  dma_desc_pa;    /* descritpor to physical address */
};

struct pru_camera {
    struct device               *dev;
    spinlock_t                  dev_lock;   /* access to pru & sensor */
    unsigned int                rst_pin;
    unsigned int                pwdn_pin;

    int                         users;
    enum prucam_state           state;

    struct v4l2_device          v4l2_dev;
    struct v4l2_ctrl_handler    ctrl_handler;

    struct v4l2_subdev          *sensor;

    struct video_device         vdev;

    /* Videobuf2 stuff */
    struct vb2_queue            vb_queue;
    struct list_head            buffers;

	struct prucam_vb_buffer		*vb_bufs[MAX_BUF_NUMBER];
	struct vb2_alloc_ctx        *vb_alloc_ctx;

    unsigned int                nbufs;          /* number of buffers allocated */
    short int                   clock_speed;    /* sensor clock xclk/pclk? */

	/* current operating parameters */
    struct v4l2_pix_format      pix_format;
    u32                         mbus_code;

    struct mutex                s_mutex;    /* access to this struct */
    int							regs_size;

    unsigned int sequence;		/* Frame sequence number */
};

static inline struct prucam_vb_buffer *vb_to_pvb(struct vb2_buffer *vb) {
	return container_of(vb, struct prucam_vb_buffer, vb_buf);
}


#endif /* pru_camera_h */
