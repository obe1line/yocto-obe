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
#include <media/v4l2-ctrls.h>

#define DRIVER_NAME "ar0144"

#define AR0144_I2C_ADDR      0x10
#define AR0144_ID_REG        0x3000
#define AR0144_ID_VAL        0x1356

#define AR0144_NUM_CTRLS	5
#define STEP_VALUE_1	1

#define AR0144_MIN_HBLANK 0
#define AR0144_MAX_HBLANK 65536
#define AR0144_MIN_VBLANK 0
#define AR0144_MAX_VBLANK 65536
#define AR0144_MIN_PIXEL_RATE 74250000
#define AR0144_MAX_PIXEL_RATE 74250000
#define AR0144_MIN_EXPOSURE 0
#define AR0144_MAX_EXPOSURE 65536
#define AR0144_MIN_ANALOG_GAIN 0
#define AR0144_MAX_ANALOG_GAIN 65536

#define AR0144_PIXEL_RATE				74250000
#define AR0144_1280_800_HBLANK_VALUE	208
#define AR0144_1280_800_VBLANK_VALUE	27
#define AR0144_1280_800_EXPOSURE_VALUE	100
#define AR0144_1280_800_ANALOG_GAIN_VALUE	100

/* timing for the sensor */
struct timing {
	// output pixel_clock (max) is 74.25MHz as a default, 1280 x 800. 6-48MHz input clock.
	__u32 pixel_rate;
	__u32 hblank;
	__u32 vblank;
	__u32 exposure;
	__u32 analog_gain;
};

struct ar0144_ctrls {
	struct v4l2_ctrl_handler handler;
	struct v4l2_ctrl *pixel_rate;
	struct v4l2_ctrl *hblank;
	struct v4l2_ctrl *vblank;
	struct v4l2_ctrl *exposure;
	struct v4l2_ctrl *analog_gain;
};


struct ar0144_state {
	struct v4l2_subdev sd;
	struct media_pad pad;
	struct mutex lock;
	struct ar0144_ctrls ctrls;
	struct timing timings;
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

// Convert a v4l2_subdev to the parent ar0144_state
static inline struct ar0144_state *to_ar0144_state(struct v4l2_subdev *sd)
{
	/* return a pointer to the state structure where sd is state->sd */
	return container_of(sd, struct ar0144_state, sd);
}

// Convert a v4l2_subdev to the parent ar0144_state
static inline struct ar0144_state *ar0144_ctrls_to_ar0144_state(struct ar0144_ctrls *ctrls)
{
	/* return a pointer to the state structure where ctrl is state->ctrls */
	return container_of(ctrls, struct ar0144_state, ctrls);
}

static inline struct ar0144_ctrls *ctrl_handler_to_ar0144_ctrls(struct v4l2_ctrl_handler *handler)
{
	/* return a pointer to the state structure where ctrl is ar0144_ctrls->handler */
	return container_of(handler, struct ar0144_ctrls, handler);
}

static void ar0144_subdev_unregistered(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	dev_info(&client->dev, "Subdev %s unregistered\n", DRIVER_NAME);
};

static int ar0144_subdev_registered(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	dev_info(&client->dev, "Subdev %s registered\n", DRIVER_NAME);
    return 0;
};

static int ar0144_subdev_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	dev_info(&client->dev, "Subdev %s open\n", DRIVER_NAME);
	return 0;
};

static int ar0144_subdev_close(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	dev_info(&client->dev, "Subdev %s close\n", DRIVER_NAME);
	return 0;
};

static const struct v4l2_subdev_internal_ops ar0144_subdev_internal_ops = {
    .registered = ar0144_subdev_registered,
	.unregistered = ar0144_subdev_unregistered,
	.open = ar0144_subdev_open,
	.close = ar0144_subdev_close,
};

static int ar0144_ctrl_subdev_log_status(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	dev_info(&client->dev, "Subdev %s log status\n", DRIVER_NAME);
	return 0;
};

static int ar0144_ctrl_subdev_subscribe_event(struct v4l2_subdev *sd, struct v4l2_fh *fh, struct v4l2_event_subscription *sub)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	dev_info(&client->dev, "Subdev %s subscribe event\n", DRIVER_NAME);
	return 0;
};

static int ar0144_event_subdev_unsubscribe(struct v4l2_subdev *sd, struct v4l2_fh *fh, struct v4l2_event_subscription *sub)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	dev_info(&client->dev, "Subdev %s unsubscribe event\n", DRIVER_NAME);
	return 0;
};

static const struct v4l2_subdev_core_ops ar0144_core_ops = {
	.log_status = ar0144_ctrl_subdev_log_status,
	.subscribe_event = ar0144_ctrl_subdev_subscribe_event,
	.unsubscribe_event = ar0144_event_subdev_unsubscribe,
};

static int ar0144_g_frame_interval(struct v4l2_subdev *sd, struct v4l2_subdev_frame_interval *fi)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	dev_info(&client->dev, "%s Get frame interval\n", DRIVER_NAME);
	return 0;
};

static int ar0144_s_frame_interval(struct v4l2_subdev *sd, struct v4l2_subdev_frame_interval *fi)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	dev_info(&client->dev, "%s Set frame interval\n", DRIVER_NAME);
	return 0;
};

static int ar0144_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	dev_info(&client->dev, "%s Set stream\n", DRIVER_NAME);
	return 0;
};

static const struct v4l2_subdev_video_ops ar0144_video_ops = {
	.g_frame_interval = ar0144_g_frame_interval,
	.s_frame_interval = ar0144_s_frame_interval,
	.s_stream = ar0144_s_stream,
};

static int ar0144_init_cfg(struct v4l2_subdev *sd, struct v4l2_subdev_state *state)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	dev_info(&client->dev, "Init cfg\n");
	return 0;
};

static int ar0144_enum_mbus_code(struct v4l2_subdev *sd, struct v4l2_subdev_state *state, struct v4l2_subdev_mbus_code_enum *code)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	dev_info(&client->dev, "Enum mbus code\n");

	/* for now, only support 1 format at index 0 */
	if (code->index != 0) {
	    return -EINVAL;
	}

    code->code = MEDIA_BUS_FMT_SBGGR12_1X12;
	return 0;
};

static int ar0144_get_fmt(struct v4l2_subdev *sd, struct v4l2_subdev_state *state, struct v4l2_subdev_format *format)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	dev_info(&client->dev, "Get format\n");
	return 0;
};

static int ar0144_set_fmt(struct v4l2_subdev *sd, struct v4l2_subdev_state *state, struct v4l2_subdev_format *format)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	dev_info(&client->dev, "Set format\n");
	return 0;
};

static int ar0144_get_selection(struct v4l2_subdev *sd, struct v4l2_subdev_state *state, struct v4l2_subdev_selection *sel)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	dev_info(&client->dev, "Get selection\n");
	return 0;
};

static int ar0144_enum_frame_size(struct v4l2_subdev *sd, struct v4l2_subdev_state *state, struct v4l2_subdev_frame_size_enum *fse)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	dev_info(&client->dev, "Enum frame size\n");

	switch (fse->index) {
    case 0:
        fse->min_width = 1280;
        fse->min_height = 800;
        break;
    default:
        return -EINVAL;
    }

	return 0;
};

static int ar0144_enum_frame_interval(struct v4l2_subdev *sd, struct v4l2_subdev_state *state, struct v4l2_subdev_frame_interval_enum *fie)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	dev_info(&client->dev, "Enum frame interval\n");

	switch (fie->index) {
	case 0:
		fie->interval.numerator = 1;
		fie->interval.denominator = 60;
		break;
	default:
		return -EINVAL;
	}

	return 0;
};

static const struct v4l2_subdev_pad_ops ar0144_pad_ops = {
	.init_cfg = ar0144_init_cfg,
	.enum_mbus_code = ar0144_enum_mbus_code,
	.get_fmt = ar0144_get_fmt,
	.set_fmt = ar0144_set_fmt,
	.get_selection = ar0144_get_selection,
	.enum_frame_size = ar0144_enum_frame_size,
	.enum_frame_interval = ar0144_enum_frame_interval,
};

static const struct v4l2_subdev_ops ar0144_subdev_ops = {
	.core = &ar0144_core_ops,
	.video = &ar0144_video_ops,
	.pad = &ar0144_pad_ops,
};

static int ar0144_subdev_link_setup(struct media_entity *entity, const struct media_pad *local, const struct media_pad *remote, u32 flags)
{
	int ret = 0;
	struct v4l2_subdev *sd = media_entity_to_v4l2_subdev(entity);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	
	dev_info(&client->dev, "TODO: link setup\n");
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

static int ar0144_g_volatile_ctrl(struct v4l2_ctrl *ctrl)
{
	struct ar0144_ctrls *ctrls = ctrl_handler_to_ar0144_ctrls(ctrl->handler);
	struct ar0144_state *state = ar0144_ctrls_to_ar0144_state(ctrls);
	struct i2c_client *client = v4l2_get_subdevdata(&state->sd);
	dev_info(&client->dev, "Get volatile control\n");
	return 0;
}

static int ar0144_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct ar0144_ctrls *ctrls = ctrl_handler_to_ar0144_ctrls(ctrl->handler);
	struct ar0144_state *state = ar0144_ctrls_to_ar0144_state(ctrls);
	struct i2c_client *client = v4l2_get_subdevdata(&state->sd);
	dev_info(&client->dev, "Set control\n");
	return 0;
}

static const struct v4l2_ctrl_ops ar0144_ctrl_ops = {
	.g_volatile_ctrl = ar0144_g_volatile_ctrl,
	.s_ctrl = ar0144_s_ctrl,
};

static int ar0144_init_timings(struct ar0144_state *state)
{
	struct i2c_client *client = v4l2_get_subdevdata(&state->sd);
	state->timings.pixel_rate = AR0144_PIXEL_RATE;
	state->timings.hblank = AR0144_1280_800_HBLANK_VALUE;
	state->timings.vblank = AR0144_1280_800_VBLANK_VALUE;
	state->timings.exposure = AR0144_1280_800_EXPOSURE_VALUE;
	state->timings.analog_gain = AR0144_1280_800_ANALOG_GAIN_VALUE;
	dev_info(&client->dev, "Initialised %s timings\n", DRIVER_NAME);
	return 0;
}

static int ar0144_init_controls(struct ar0144_state *state)
{
	const struct v4l2_ctrl_ops *ops = &ar0144_ctrl_ops;
	struct ar0144_ctrls *ctrls = &state->ctrls;
	struct v4l2_ctrl_handler *ctrl_handler = &ctrls->handler;
	struct i2c_client *client = v4l2_get_subdevdata(&state->sd);
	int ret = 0;

	// initialise the control handler for all controls
	v4l2_ctrl_handler_init(ctrl_handler, AR0144_NUM_CTRLS);
	if (ctrl_handler->error) {
		dev_err(&client->dev, "v4l2_ctrl_handler_init %s failed: %#010X\n", DRIVER_NAME, ctrl_handler->error);
		return ctrl_handler->error;
	}

	/* hblank */
	ctrls->hblank = v4l2_ctrl_new_std(ctrl_handler, ops, V4L2_CID_HBLANK, 
						AR0144_MIN_HBLANK, AR0144_MAX_HBLANK, STEP_VALUE_1, state->timings.hblank);
	if (ctrl_handler->error) {
		dev_err(&client->dev, "hblank v4l2_ctrl_new_std %s failed: %#010X\n", DRIVER_NAME, ctrl_handler->error);
		goto ctrl_error;
	}

	/* vblank */
	ctrls->vblank = v4l2_ctrl_new_std(ctrl_handler, ops, V4L2_CID_VBLANK, 
						AR0144_MIN_VBLANK, AR0144_MAX_VBLANK, STEP_VALUE_1, state->timings.vblank);
	if (ctrl_handler->error) {
		dev_err(&client->dev, "vblank v4l2_ctrl_new_std %s failed: %#010X\n", DRIVER_NAME, ctrl_handler->error);
		goto ctrl_error;
	}

	/* pixel rate */
	ctrls->pixel_rate = v4l2_ctrl_new_std(ctrl_handler, ops, V4L2_CID_PIXEL_RATE, 
						AR0144_MIN_PIXEL_RATE, AR0144_MAX_PIXEL_RATE, STEP_VALUE_1, state->timings.pixel_rate);
	if (ctrl_handler->error) {
		dev_err(&client->dev, "pixel_rate v4l2_ctrl_new_std %s failed: %#010X\n", DRIVER_NAME, ctrl_handler->error);
		goto ctrl_error;
	}

	/* exposure */
	ctrls->exposure = v4l2_ctrl_new_std(ctrl_handler, ops, V4L2_CID_EXPOSURE, 
						AR0144_MIN_EXPOSURE, AR0144_MAX_EXPOSURE, STEP_VALUE_1, state->timings.exposure);
	if (ctrl_handler->error) {
		dev_err(&client->dev, "exposure v4l2_ctrl_new_std %s failed: %#010X\n", DRIVER_NAME, ctrl_handler->error);
		goto ctrl_error;
	}
	
	/* analog gain */
	ctrls->analog_gain = v4l2_ctrl_new_std(ctrl_handler, ops, V4L2_CID_ANALOGUE_GAIN, 
						AR0144_MIN_ANALOG_GAIN, AR0144_MAX_ANALOG_GAIN, STEP_VALUE_1, state->timings.analog_gain);
	if (ctrl_handler->error) {
		dev_err(&client->dev, "analog_gain v4l2_ctrl_new_std %s failed: %#010X\n", DRIVER_NAME, ctrl_handler->error);
		goto ctrl_error;
	}

	ctrls->hblank->flags |= V4L2_CTRL_FLAG_READ_ONLY;
	ctrls->vblank->flags |= V4L2_CTRL_FLAG_READ_ONLY;
	ctrls->pixel_rate->flags |= V4L2_CTRL_FLAG_READ_ONLY;
	ctrls->exposure->flags |= V4L2_CTRL_FLAG_READ_ONLY;
	ctrls->analog_gain->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	state->sd.ctrl_handler = ctrl_handler;

	return ret;

ctrl_error:
	ret = ctrl_handler->error;
	v4l2_ctrl_handler_free(ctrl_handler);
	return ret;
}

/*
Probe is called when the kernel matches a device tree node with the driver compatibile names.
After allocating memory for the device, the function checks the device ID to ensure it is the correct device.
It then registers the device with the V4L2 and media frameworks, and sets up the device.
*/
static int ar0144_probe(struct i2c_client *client)
{
	struct ar0144_state *state;
	int ret = 0;

	state = devm_kzalloc(&client->dev, sizeof(*state), GFP_KERNEL);
	if (!state) {
		return -ENOMEM;
	}

	/* Initialise the V4L2 structure and set the flags for a subdevice sensor */
	v4l2_i2c_subdev_init(&state->sd, client, &ar0144_subdev_ops);
	state->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE | V4L2_SUBDEV_FL_HAS_EVENTS;
	state->sd.internal_ops = &ar0144_subdev_internal_ops;
	state->sd.entity.function = MEDIA_ENT_F_CAM_SENSOR;
	state->sd.entity.ops = &ar0144_media_ops;
	state->sd.entity.obj_type = MEDIA_ENTITY_TYPE_V4L2_SUBDEV;
	state->sd.entity.name = DRIVER_NAME;

	mutex_init(&state->lock);

	dev_info(&client->dev, "Initialised %s state\n", DRIVER_NAME);

	state->pad.flags = MEDIA_PAD_FL_SOURCE;
	ret = media_entity_pads_init(&state->sd.entity, 1, &state->pad);
	if (ret) {
		dev_err(&client->dev, "Probe %s unable to init media pads: %#010X\n", DRIVER_NAME, ret);
		goto exit_probe;
	}

	ret = ar0144_init_timings(state);
	if (ret < 0) {
		dev_err(&client->dev, "Probe %s unable to init timings: %#010X\n", DRIVER_NAME, ret);
		goto exit_probe;
	}

	ret = ar0144_init_controls(state);
	if (ret < 0) {
		dev_err(&client->dev, "Probe %s unable to init controls: %#010X\n", DRIVER_NAME, ret);
		goto exit_probe;
	}

	ret = v4l2_async_register_subdev_sensor(&state->sd);
	if (ret < 0) {
		dev_err(&client->dev, "Probe %s unable to resister async: %#010X\n", DRIVER_NAME, ret);
		goto exit_probe;
	}

	dev_info(&client->dev, "Probe %s successful\n", DRIVER_NAME);
	return ret;

exit_probe:
	// TODO: free resources as required
	return ret;
}

/*
Remove the driver and clean up.
*/
static void ar0144_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ar0144_state *state = to_ar0144_state(sd);
	v4l2_async_unregister_subdev(sd);
	mutex_destroy(&state->lock);
	dev_info(&client->dev, "Removed %s driver\n", DRIVER_NAME);
}

/* create the init and exit functions */
module_i2c_driver(ar0144_i2c_driver);

MODULE_AUTHOR("Christopher Storah <christopher.storah@leicabiosystems.com>");
MODULE_ALIAS("i2c:ar0144");
MODULE_DESCRIPTION("ON Semiconductor AR0144 Camera Sensor");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("0.1");
