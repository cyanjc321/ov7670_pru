/*
 * Remote processor messaging - prucam client driver
 *
 * Copyright (C) 2011 Texas Instruments, Inc.
 * Copyright (C) 2011 Google, Inc.
 *
 * Ohad Ben-Cohen <ohad@wizery.com>
 * Brian Swetland <swetland@google.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/rpmsg.h>

#include "pru_camera_reg.h"

static volatile int flag;	//interrupt flag
static struct rpmsg_channel	*rp_prucam;
static const unsigned long timeout = 10;


static void rpmsg_prucam_cb(struct rpmsg_channel *rpdev, void *data, int len,
						void *priv, u32 src)
{
	struct prucam_query *reply = dev_get_drvdata(&rpdev->dev);

	dev_info(&rpdev->dev, "incoming msg (src: 0x%x)\n", src);

	print_hex_dump(KERN_DEBUG, __func__, DUMP_PREFIX_NONE, 16, 1,
		       data, len,  true);

	*reply = *((struct prucam_query *)data);

	flag = 1;

	if (len != sizeof(struct prucam_query))
		flag = -1;
}

static int rpmsg_prucam_io(struct rpmsg_channel *rpdev, struct prucam_query *query) {
	struct prucam_query *reply = dev_get_drvdata(&rpdev->dev);
	unsigned long start;
	int ret;

	ret = rpmsg_send(rpdev, query, sizeof(struct prucam_query));
	if (ret) {
		dev_err(&rpdev->dev, "rpmsg_send failed: %d\n", ret);
		return ret;
	}

	start = jiffies;
	while (!flag) {
		if (time_after(jiffies, start + timeout))
			return -ETIMEDOUT;
		cpu_relax();
	};

	*query = *reply;

	if (flag < 0)
		return -EBADMSG;

	return 0;
}

int prucam_reg_read(unsigned int addr, unsigned int *data) {
	struct prucam_query query;
	int ret;

	query.op = reg_read;
	query.addr = addr;
	query.data = 0;

	ret = rpmsg_prucam_io(rp_prucam, &query);

	if ((query.data == 0xffffffff) || (query.addr != addr) || (query.op != reg_read))
		return -EBADMSG;

	*data = query.data;
	dev_info(&rp_prucam->dev, "read reg@%x: %d.\n", addr, *data);

	return ret;
}
EXPORT_SYMBOL(prucam_reg_read);

int prucam_reg_write(unsigned int addr, unsigned int data) {
	struct prucam_query query;
	int ret;

	query.op = reg_write;
	query.addr = addr;
	query.data = data;

	dev_info(&rp_prucam->dev, "setting reg@%x to %d.\n", addr, data);

	ret = rpmsg_prucam_io(rp_prucam, &query);

	if ((query.data == 0xffffffff) || (query.addr != addr) || (query.op != reg_write))
		return -EBADMSG;

	return ret;
}
EXPORT_SYMBOL(prucam_reg_write);

int prucam_buf_queue(unsigned int pt) {
	struct prucam_query query;
	int ret;

	query.op = reg_write;
	query.addr = REG_BUFPT;
	query.data = pt;

	dev_info(&rp_prucam->dev, "queueing buffer@%x to PRU.\n", pt);

	ret = rpmsg_prucam_io(rp_prucam, &query);

	if ((query.data == 0xffffffff) || (query.addr != REG_BUFPT) || (query.op != reg_write))
		return -EBADMSG;

	return ret;
}
EXPORT_SYMBOL(prucam_buf_queue);

static int rpmsg_prucam_probe(struct rpmsg_channel *rpdev)
{
	struct prucam_query *reply;

	dev_info(&rpdev->dev, "new channel: 0x%x -> 0x%x!\n",
					rpdev->src, rpdev->dst);

	reply = devm_kzalloc(&rpdev->dev, sizeof(struct prucam_query), GFP_KERNEL);
	if (!reply)
		return -ENOMEM;

	rp_prucam = rpdev;

	dev_set_drvdata(&rpdev->dev, reply);

	flag = 0;

	return 0;
}

static void rpmsg_prucam_remove(struct rpmsg_channel *rpdev)
{
	dev_info(&rpdev->dev, "rpmsg prucam client driver is removed\n");
}


static struct rpmsg_device_id rpmsg_prucam_id_table[] = {
	{ .name	= "rpmsg-pru-camera" },
	{ },
};
MODULE_DEVICE_TABLE(rpmsg, rpmsg_prucam_id_table);

static struct rpmsg_driver rpmsg_prucam_client = {
	.drv.name	= KBUILD_MODNAME,
	.drv.owner	= THIS_MODULE,
	.id_table	= rpmsg_prucam_id_table,
	.probe		= rpmsg_prucam_probe,
	.callback	= rpmsg_prucam_cb,
	.remove		= rpmsg_prucam_remove,
};

static int __init rpmsg_prucam_init(void)
{
	return register_rpmsg_driver(&rpmsg_prucam_client);
}
module_init(rpmsg_prucam_init);

static void __exit rpmsg_prucam_exit(void)
{
	unregister_rpmsg_driver(&rpmsg_prucam_client);
}
module_exit(rpmsg_prucam_exit);

MODULE_DESCRIPTION("Remote processor messaging prucam client driver");
MODULE_LICENSE("GPL v2");
