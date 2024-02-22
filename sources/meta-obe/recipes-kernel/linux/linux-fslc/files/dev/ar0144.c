// SPDX-License-Identifier: GPL-2.0-only
/*
 * Driver for the AR0144 sensor by ON Semiconductor
 *
 * Copyright (C) 2024, Leica Biosystems Pty Ltd.
 * Copyright (C) 2024, Christopher Storah <christopher.storah@leicabiosystems.com>
 *
 */

#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/media.h>
#include <linux/module.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-subdev.h>

#define DRIVER_NAME "ar0144"

#define AR0144_I2C_ADDR      0x10
#define AR0144_ID_REG        0x3000
#define AR0144_ID_VAL        0x1356

struct ar0144_device {
	struct device *dev;
	struct i2c_client *client;
	struct v4l2_subdev sd;
	struct media_pad pad;
	struct mutex lock;
};

/*
libcamera support:
Mandatory Requirements
----------------------

.. _V4L2_CID_ANALOGUE_GAIN: https://www.kernel.org/doc/html/latest/userspace-api/media/v4l/ext-ctrls-image-source.html
.. _V4L2_CID_EXPOSURE: https://www.kernel.org/doc/html/latest/userspace-api/media/v4l/control.html
.. _V4L2_CID_HBLANK: https://www.kernel.org/doc/html/latest/userspace-api/media/v4l/ext-ctrls-image-source.html
.. _V4L2_CID_PIXEL_RATE: https://www.kernel.org/doc/html/latest/userspace-api/media/v4l/ext-ctrls-image-process.html
.. _V4L2_CID_VBLANK: https://www.kernel.org/doc/html/latest/userspace-api/media/v4l/ext-ctrls-image-source.html

Optional Requirements
---------------------
.. _V4L2_CID_CAMERA_ORIENTATION: https://www.kernel.org/doc/html/latest/userspace-api/media/v4l/ext-ctrls-camera.html
.. _V4L2_CID_CAMERA_SENSOR_ROTATION: https://www.kernel.org/doc/html/latest/userspace-api/media/v4l/ext-ctrls-camera.html
.. _V4L2_CID_HFLIP: https://www.kernel.org/doc/html/latest/userspace-api/media/v4l/control.html
.. _V4L2_CID_VFLIP: https://www.kernel.org/doc/html/latest/userspace-api/media/v4l/control.html

The controls must be writable from userspace. In case of a RAW Bayer sensors,
drivers should correctly report if vertical/horizontal flips modify the Bayer
pattern ordering by reporting the `V4L2_CTRL_FLAG_MODIFY_LAYOUT` control flag.

The sensor driver should implement support for the V4L2 Selection API,
specifically it should implement support for the
`VIDIOC_SUBDEV_G_SELECTION`_ ioctl with support for the following selection
targets:

.. _VIDIOC_SUBDEV_G_SELECTION: https://www.kernel.org/doc/html/latest/userspace-api/media/v4l/vidioc-subdev-g-selection.html#c.V4L.VIDIOC_SUBDEV_G_SELECTION

.. _V4L2_SEL_TGT_CROP_BOUNDS: https://www.kernel.org/doc/html/latest/userspace-api/media/v4l/v4l2-selection-targets.html
.. _V4L2_SEL_TGT_CROP_DEFAULT: https://www.kernel.org/doc/html/latest/userspace-api/media/v4l/v4l2-selection-targets.html
.. _V4L2_SEL_TGT_CROP: https://www.kernel.org/doc/html/latest/userspace-api/media/v4l/v4l2-selection-targets.html

*/

// Convert a v4l2_subdev to the parent ar0144_device
static inline struct ar0144_device *to_ar0144_device(struct v4l2_subdev *sd)
{
	return container_of(sd, struct ar0144_device, sd);
};

static void ar0144_subdev_unregistered(struct v4l2_subdev *sd)
{
	struct ar0144_device *jd = to_ar0144_device(sd);
	
	v4l2_async_unregister_subdev(sd);
	dev_info(jd->dev, "Subdev unregistered\n");

};

static int ar0144_subdev_registered(struct v4l2_subdev *sd)
{
	int ret = 0;
	struct ar0144_device *jd = to_ar0144_device(sd);

	// register as a sub-device
	ret = v4l2_device_register_subdev(sd->v4l2_dev, sd);
	if (ret <0) {
		dev_info(jd->dev, "Unable to register subdev: %#010X\n", ret);
		return ret;
	}

	dev_info(jd->dev, "Subdev registered\n");
    return 0;
};

static const struct v4l2_subdev_internal_ops ar0144_subdev_internal_ops = {
    .registered = ar0144_subdev_registered,
	.unregistered = ar0144_subdev_unregistered,
};

static const struct v4l2_subdev_ops ar0144_subdev_ops = {
	// .core = &ar0144_core_ops,
	// .video = &ar0144_video_ops,
	// .pad = &ar0144_pad_ops,
};

static int ar0144_subdev_link_setup(struct media_entity *entity, const struct media_pad *local, const struct media_pad *remote, u32 flags)
{
	int ret = 0;
	struct v4l2_subdev *sd = media_entity_to_v4l2_subdev(entity);
	// struct ar0144_device *jd = to_ar0144_device(sd);
	
	dev_info(sd->dev, "TODO: link setup\n");
	return ret;
};

static const struct media_entity_operations ar0144_media_ops = {
	.link_validate = v4l2_subdev_link_validate,
	.link_setup = ar0144_subdev_link_setup,
};

static const struct i2c_device_id ar0144_idtable[] = {
      { "ar0144", 0 },
      { /* sentinel */ }
};
MODULE_DEVICE_TABLE(i2c, ar0144_idtable);

/* Table used to match the device tree with the driver. */
static const struct of_device_id ar0144_of_match[] = {
	{ .compatible = "onnn,ar0144" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, ar0144_of_match);

static int ar0144_probe(struct i2c_client *client);
static void ar0144_remove(struct i2c_client *client);

/* I2C driver details including functions for probe and remove. */
static struct i2c_driver ar0144_i2c_driver = {
	.driver = {
		.name   = DRIVER_NAME,
		.of_match_table = of_match_ptr(ar0144_of_match),
	},
	.id_table       = ar0144_idtable,
	.probe          = ar0144_probe,
	.remove         = ar0144_remove,
};

/*
Probe is called when the kernel matches a device tree node with the driver compatibile names.
After allocating memory for the device, the function checks the device ID to ensure it is the correct device.
It then registers the device with the V4L2 and media frameworks, and sets up the device.
*/
static int ar0144_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct ar0144_device *devm;
	struct v4l2_subdev *sd = &devm->sd;
	int ret = 0;

	devm = devm_kzalloc(dev, sizeof(*devm), GFP_KERNEL);
	if (!devm) {
		return -ENOMEM;
	}

	/* Initialise the V4L2 structure and set the flags for a subdevice sensor */
	v4l2_i2c_subdev_init(sd, client, &ar0144_subdev_ops);
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE | V4L2_SUBDEV_FL_HAS_EVENTS;
	sd->internal_ops = &ar0144_subdev_internal_ops;
	sd->entity.function = MEDIA_ENT_F_CAM_SENSOR;
	sd->entity.ops = &ar0144_media_ops;
	sd->entity.obj_type = MEDIA_ENTITY_TYPE_V4L2_SUBDEV;
	sd->entity.name = DRIVER_NAME;

	mutex_init(&devm->lock);

	ret = v4l2_async_register_subdev(sd);
	if (ret < 0) {
		dev_err(dev, "Probe unable to resister async: %#010X\n", ret);
		goto error;
	}

	dev_info(dev, "Probe successful\n");
	return ret;

error:
	mutex_destroy(&devm->lock);

	return ret;
}

/*
Remove the driver and clean up.
*/
static void ar0144_remove(struct i2c_client *client)
{
	dev_info(&client->dev, "Removed %s driver\n", DRIVER_NAME);
}

/* create the init and exit functions */
module_i2c_driver(ar0144_i2c_driver);

MODULE_AUTHOR("Christopher Storah <christopher.storah@leicabiosystems.com>");
MODULE_ALIAS("i2c:ar0144");
MODULE_DESCRIPTION("ON Semiconductor AR0144 Camera Sensor");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("0.1");
