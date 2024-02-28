// SPDX-License-Identifier: GPL-2.0-only
/*
 * Driver for the AR0144 sensor by ON Semiconductor
 *
 * Copyright (C) 2024, Leica Biosystems Pty Ltd.
 * Copyright (C) 2024, Christopher Storah <christopher.storah@leicabiosystems.com>
 *
 */

// TODO: remove this when the driver is complete
#define DEBUG 1

#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/media.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-fwnode.h>

#define DRIVER_NAME "ar0144"

// default I2C address
#define AR0144_I2C_ADDR      0x10

// register addresses
#define AR0144_ID_REG               0x3000
#define AR0144_TEST_PATTERN_MODE    0x3070

// sensor identifier
#define AR0144_ID_VAL        0x0356

// values
#define AR0144_MIN_HBLANK 0
#define AR0144_MAX_HBLANK 65536
#define AR0144_MIN_VBLANK 0
#define AR0144_MAX_VBLANK 65536
#define AR0144_MIN_PIXEL_RATE 74250000
#define AR0144_MAX_PIXEL_RATE 74250000
#define AR0144_MIN_EXPOSURE 0
#define AR0144_MAX_EXPOSURE 1023
#define AR0144_MIN_ANALOG_GAIN 0
#define AR0144_MAX_ANALOG_GAIN 1023
#define AR0144_PIXEL_RATE				74250000
#define AR0144_1280_800_HBLANK_VALUE	208
#define AR0144_1280_800_VBLANK_VALUE	27
#define AR0144_1280_800_EXPOSURE_VALUE	100
#define AR0144_1280_800_ANALOG_GAIN_VALUE	0

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
	struct v4l2_ctrl *orientation;
	struct v4l2_ctrl *test_pattern;
};

// supported test patterns as per MIPI CCS v1.1 section 10 "Test Modes"
static const char * const test_pattern_menu[] = {
	"No pattern",
	"Solid color",
	"100% Color bars",
	"Fade-to-grey color bars",
};

struct ar0144_reg_val_pair {
	u16 reg;
	u16 val;
};

static const struct ar0144_reg_val_pair ar0144_stream_enable[] = {
//	{0x3028, 0x0010},	/* ??? */
	{0x301A, 0x005C},	/* start stream */
};

static const struct ar0144_reg_val_pair ar0144_stream_disable[] = {
	{0x301A, 0x0058},	/* stop stream */
};

struct ar0144_state {
	struct v4l2_subdev sd;
	struct media_pad pad;
	struct mutex lock;
	struct ar0144_ctrls ctrls;
	struct timing timings;
	struct v4l2_mbus_framefmt fmt;
	struct regmap *regs;
};

// The AR0144 sensor has mostly a 16-bit register interface. Handle 8-bit and 32-bit with another regmap?
 const struct regmap_config ar0144_regmap_config = {
	.reg_bits = 16,
	.val_bits = 16,
	.cache_type = REGCACHE_NONE,
};

/*
libcamera support:
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
	dev_info(&client->dev, "Subdev unregistered\n");
};

static int ar0144_subdev_registered(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	dev_info(&client->dev, "Subdev registered\n");
    return 0;
};

static int ar0144_subdev_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	dev_info(&client->dev, "Subdev open\n");
	return 0;
};

static int ar0144_subdev_close(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	dev_info(&client->dev, "Subdev close\n");
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
	struct ar0144_state *state = to_ar0144_state(sd);
	dev_info(&client->dev, "Sensor status:\n");
	dev_info(&client->dev, "  Test pattern: %d\n", state->ctrls.test_pattern->val);
	return 0;
};

// static int ar0144_ctrl_subdev_subscribe_event(struct v4l2_subdev *sd, struct v4l2_fh *fh, struct v4l2_event_subscription *sub)
// {
// 	struct i2c_client *client = v4l2_get_subdevdata(sd);
// 	dev_info(&client->dev, "Subdev subscribe event\n");

// 	if (sub->type == V4L2_EVENT_CTRL) {
// 		dev_info(&client->dev, "Subdev subscribe event: V4L2_EVENT_CTRL, id=%#010X\n");
// 		switch(sub->id){
// 			case V4L2_CID_EXPOSURE:
// 				dev_info(&client->dev, "Subdev subscribe event: V4L2_CID_EXPOSURE\n");
// 				break;
// 		}
// 	}
// 	else {
// 		dev_info(&client->dev, "Subdev subscribe event: unknown\n");
// 	}
// 	return 0;
// };

// static int ar0144_event_subdev_unsubscribe(struct v4l2_subdev *sd, struct v4l2_fh *fh, struct v4l2_event_subscription *sub)
// {
// 	struct i2c_client *client = v4l2_get_subdevdata(sd);
// 	dev_info(&client->dev, "Subdev unsubscribe event\n");
// 	return 0;
// };

static int ar0144_write_register_array(struct ar0144_state *state,
				       const struct ar0144_reg_val_pair *regs,
				       unsigned int num_pairs)
{
	unsigned int index = 0;
	int ret = 0;

	for (index = 0; index < num_pairs; ++index, ++regs) {
		ret = regmap_write(state->regs, regs->reg, regs->val);
		if (ret < 0)
			break;
	}

	return ret;
}

int validate_sensor_identifier(struct ar0144_state *state)
{
	struct i2c_client *client = v4l2_get_subdevdata(&state->sd);
	unsigned int val = 0;
	int ret = 0;

	ret = regmap_read(state->regs, AR0144_ID_REG, &val);
	if (ret < 0) {
		dev_err(&client->dev, "Unable to read sensor registers: error %#010X\n", ret);
	}
	else {
		dev_info(&client->dev, "Detected sensor identifier of %#06X\n", val);
		if (val != AR0144_ID_VAL) {
			dev_err(&client->dev, "Unsupported sensor\n");
			ret = -ENODEV;
		}
	}

	return ret;
}

static int ar0144_subdev_s_power(struct v4l2_subdev *sd, int on)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ar0144_state *ar_state = to_ar0144_state(sd);
	int ret = 0;
	
	dev_info(&client->dev, "Subdev power %s\n", on ? "on" : "off");

	// power on handled externally via a script for now so just detect and setup registers
	
	mutex_lock(&ar_state->lock);
	if (on) {
		// power on
		// TODO: control power to the sensor

		// check the sensor ID
		ret = validate_sensor_identifier(ar_state);
		if (ret < 0) {
			dev_err(&client->dev, "Unsupported sensor\n");
			goto power_out;
		}

		dev_info(&client->dev, "Power on\n");
	}
	else {
		// power off
		dev_info(&client->dev, "Power off\n");
	}

power_out:
	mutex_unlock(&ar_state->lock);
	return ret;
};

long int ar0144_subdev_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	dev_info(&client->dev, "Subdev ioctl. cmd = %#010X\n", cmd);
	return 0;
};


static const struct v4l2_subdev_core_ops ar0144_core_ops = {
	.ioctl = ar0144_subdev_ioctl,
	.s_power = ar0144_subdev_s_power,
	.log_status = ar0144_ctrl_subdev_log_status,
	// .subscribe_event = ar0144_ctrl_subdev_subscribe_event,		// only needed if the sensor generates events
	// .unsubscribe_event = ar0144_event_subdev_unsubscribe,
};

static int ar0144_g_frame_interval(struct v4l2_subdev *sd, struct v4l2_subdev_frame_interval *fi)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	dev_info(&client->dev, "Get frame interval\n");
	// TODO: fetch from state
	fi->interval.numerator = 1;
	fi->interval.denominator = 60;
	return 0;
};

static int ar0144_s_frame_interval(struct v4l2_subdev *sd, struct v4l2_subdev_frame_interval *fi)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	dev_dbg(&client->dev, "Set frame interval. numerator: %d, denominator: %d\n",
		fi->interval.numerator, fi->interval.denominator);
	// TODO: store in state
	return 0;
};

static int ar0144_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ar0144_state *state = to_ar0144_state(sd);
	int ret = 0;

	dev_dbg(&client->dev, "Set stream: enable=%d\n", enable);

	if (enable) {
		ret = ar0144_write_register_array(state, ar0144_stream_enable, ARRAY_SIZE(ar0144_stream_enable));
		if (ret < 0) {
			dev_err(&client->dev, "Error writing stream enable registers\n");
		}
	}
	else {
		ret = ar0144_write_register_array(state, ar0144_stream_disable, ARRAY_SIZE(ar0144_stream_disable));
		if (ret < 0) {
			dev_err(&client->dev, "Error writing stream disable registers\n");
		}
	}

	return ret;
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

#define AR0144_WIDTH_MIN 	8u
#define AR0144_WIDTH_MAX	1292u
#define AR0144_HEIGHT_MIN	8u
#define AR0144_HEIGHT_MAX	812u

int ar0144_set_fmt_to_defaults(struct ar0144_state *state)
{
	struct v4l2_mbus_framefmt *fmt = &state->fmt;

	fmt->width = 1080;
	fmt->height = 800;
	fmt->code = MEDIA_BUS_FMT_SRGGB12_1X12;
	fmt->field = V4L2_FIELD_NONE;
	fmt->colorspace = V4L2_COLORSPACE_SRGB;
	// fmt->colorspace = V4L2_COLORSPACE_SRGB;
	// fmt->ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT;
	// fmt->quantization = V4L2_QUANTIZATION_FULL_RANGE;
	// fmt->xfer_func = V4L2_XFER_FUNC_DEFAULT;
	return 0;
};

static void ar0144_adj_fmt(struct v4l2_mbus_framefmt *fmt)
{
	fmt->width = clamp(ALIGN(fmt->width, 4), AR0144_WIDTH_MIN, AR0144_WIDTH_MAX);
	fmt->height = clamp(ALIGN(fmt->height, 4), AR0144_HEIGHT_MIN, AR0144_HEIGHT_MAX);
	fmt->code = MEDIA_BUS_FMT_SRGGB12_1X12;
	fmt->field = V4L2_FIELD_NONE;
	// fmt->colorspace = V4L2_COLORSPACE_RAW;
	fmt->colorspace = V4L2_COLORSPACE_SRGB;
	// fmt->ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT;
	// fmt->quantization = V4L2_QUANTIZATION_FULL_RANGE;
	// fmt->xfer_func = V4L2_XFER_FUNC_DEFAULT;
}

static int ar0144_enum_mbus_code(struct v4l2_subdev *sd, struct v4l2_subdev_state *state, struct v4l2_subdev_mbus_code_enum *code)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ar0144_state *ar_state = to_ar0144_state(sd);

	dev_info(&client->dev, "Enum mbus code\n");

	/* for now, only support 1 format at index 0 */
	if (code->index != 0) {
	    return -EINVAL;
	}

    code->code = ar_state->fmt.code;
	dev_info(&client->dev, "MEDIA_BUS_FMT_SRGGB12_1X12 = %#010X, code = %#010X\n", MEDIA_BUS_FMT_SRGGB12_1X12, code->code);
	return 0;
};

static int ar0144_get_fmt(struct v4l2_subdev *sd, struct v4l2_subdev_state *state, struct v4l2_subdev_format *fmt)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ar0144_state *ar_state = to_ar0144_state(sd);

	mutex_lock(&ar_state->lock);
	fmt->format = ar_state->fmt;
	mutex_unlock(&ar_state->lock);

	dev_info(&client->dev, "Get format. fmt->code=%#010X\n", fmt->format.code);
	dev_info(&client->dev, "Get format. fmt->width=%d, fmt->height=%d\n", fmt->format.width, fmt->format.height);

	return 0;
};

static int ar0144_set_fmt(struct v4l2_subdev *sd, struct v4l2_subdev_state *state, struct v4l2_subdev_format *fmt)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct ar0144_state *ar_state = to_ar0144_state(sd);
	struct v4l2_mbus_framefmt *mbus_fmt = &fmt->format;

	dev_info(&client->dev, "Set format. code=%#010X\n", mbus_fmt->code);
	dev_info(&client->dev, "Set format. width=%d, height=%d\n", mbus_fmt->width, mbus_fmt->height);
	dev_info(&client->dev, "Set format. field=%d, colorspace=%d\n", mbus_fmt->field, mbus_fmt->colorspace);

	ar0144_adj_fmt(mbus_fmt);

	mutex_lock(&ar_state->lock);
	ar_state->fmt = *mbus_fmt;
	mutex_unlock(&ar_state->lock);

	return 0;
};

static int ar0144_get_selection(struct v4l2_subdev *sd, struct v4l2_subdev_state *state, struct v4l2_subdev_selection *sel)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	switch (sel->target) {
	case V4L2_SEL_TGT_NATIVE_SIZE:
		sel->r.left = 0;
		sel->r.top = 0;
		sel->r.width = 1280;
		sel->r.height = 800;
		dev_info(&client->dev, "Get selection V4L2_SEL_TGT_NATIVE_SIZE hardcoded to 1080, 800\n");
		break;
	case V4L2_SEL_TGT_CROP_BOUNDS:
		sel->r.left = 0;
		sel->r.top = 0;
		sel->r.width = 1280;
		sel->r.height = 800;
		dev_info(&client->dev, "Get selection V4L2_SEL_TGT_CROP_BOUNDS hardcoded to 1080, 800\n");
		break;
	case V4L2_SEL_TGT_CROP_DEFAULT:
		sel->r.left = 0;
		sel->r.top = 0;
		sel->r.width = 1280;
		sel->r.height = 800;
		dev_info(&client->dev, "Get selection V4L2_SEL_TGT_CROP_DEFAULT hardcoded to 1080, 800\n");
		break;
	case V4L2_SEL_TGT_CROP:
		sel->r.left = 0;
		sel->r.top = 0;
		sel->r.width = 1280;
		sel->r.height = 800;
		dev_info(&client->dev, "Get selection V4L2_SEL_TGT_CROP hardcoded to 1080, 800\n");
		break;

	default:
		return -EINVAL;
	}

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
		fse->max_width = 1280;
		fse->max_height = 800;
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
	
	dev_dbg(&client->dev, "Notification: link setup\n");
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
	int ret = 0;

	dev_info(&client->dev, "Set control %#010X\n", ctrl->id);
	switch (ctrl->id) {
		case V4L2_CID_HBLANK:
			dev_info(&client->dev, "V4L2_CID_HBLANK\n");	
			break;
		case V4L2_CID_VBLANK:
			dev_info(&client->dev, "V4L2_CID_VBLANK\n");	
			break;
		case V4L2_CID_ANALOGUE_GAIN:
			dev_info(&client->dev, "V4L2_CID_ANALOGUE_GAIN\n");	
			break;
		case V4L2_CID_GAIN:
			dev_info(&client->dev, "V4L2_CID_GAIN\n");	
			break;
		case V4L2_CID_RED_BALANCE:
			dev_info(&client->dev, "V4L2_CID_RED_BALANCE\n");	
			break;
		case V4L2_CID_BLUE_BALANCE:
			dev_info(&client->dev, "V4L2_CID_BLUE_BALANCE\n");	
			break;
		case V4L2_CID_EXPOSURE:
			dev_info(&client->dev, "V4L2_CID_EXPOSURE\n");	
			break;
		case V4L2_CID_TEST_PATTERN:
			dev_info(&client->dev, "V4L2_CID_TEST_PATTERN\n");
			ret = regmap_write(state->regs, AR0144_TEST_PATTERN_MODE, ctrl->val);
			break;
		default:
			dev_err(&client->dev, "Unsupported control %#010X\n", ctrl->id);
			ret = -EINVAL;
			break;
	}

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
	dev_info(&client->dev, "Initialised timings\n");
	return 0;
}

static int ar0144_init_controls(struct ar0144_state *state)
{
	const struct v4l2_ctrl_ops *ops = &ar0144_ctrl_ops;
	struct ar0144_ctrls *ctrls = &state->ctrls;
	struct v4l2_ctrl_handler *ctrl_handler = &ctrls->handler;
	struct i2c_client *client = v4l2_get_subdevdata(&state->sd);
	struct v4l2_fwnode_device_properties props;
	int ret = 0;

	#define AR0144_NUM_CTRLS	7
	#define STEP_VALUE_1	1

	// initialise the control handler for all controls
	v4l2_ctrl_handler_init(ctrl_handler, AR0144_NUM_CTRLS);
	if (ctrl_handler->error) {
		dev_err(&client->dev, "v4l2_ctrl_handler_init failed: %#010X\n", ctrl_handler->error);
		return ctrl_handler->error;
	}

	/* hblank */
	ctrls->hblank = v4l2_ctrl_new_std(ctrl_handler, ops, V4L2_CID_HBLANK, 
						AR0144_MIN_HBLANK, AR0144_MAX_HBLANK, STEP_VALUE_1, state->timings.hblank);
	if (ctrl_handler->error) {
		dev_err(&client->dev, "hblank v4l2_ctrl_new_std failed: %#010X\n", ctrl_handler->error);
		goto ctrl_error;
	}

	/* vblank */
	ctrls->vblank = v4l2_ctrl_new_std(ctrl_handler, ops, V4L2_CID_VBLANK, 
						AR0144_MIN_VBLANK, AR0144_MAX_VBLANK, STEP_VALUE_1, state->timings.vblank);
	if (ctrl_handler->error) {
		dev_err(&client->dev, "vblank v4l2_ctrl_new_std failed: %#010X\n", ctrl_handler->error);
		goto ctrl_error;
	}

	/* pixel rate */
	ctrls->pixel_rate = v4l2_ctrl_new_std(ctrl_handler, ops, V4L2_CID_PIXEL_RATE, 
						AR0144_MIN_PIXEL_RATE, AR0144_MAX_PIXEL_RATE, STEP_VALUE_1, state->timings.pixel_rate);
	if (ctrl_handler->error) {
		dev_err(&client->dev, "pixel_rate v4l2_ctrl_new_std failed: %#010X\n", ctrl_handler->error);
		goto ctrl_error;
	}

	/* exposure */
	ctrls->exposure = v4l2_ctrl_new_std(ctrl_handler, ops, V4L2_CID_EXPOSURE, 
						AR0144_MIN_EXPOSURE, AR0144_MAX_EXPOSURE, STEP_VALUE_1, state->timings.exposure);
	if (ctrl_handler->error) {
		dev_err(&client->dev, "exposure v4l2_ctrl_new_std failed: %#010X\n", ctrl_handler->error);
		goto ctrl_error;
	}
	
	/* analog gain */
	ctrls->analog_gain = v4l2_ctrl_new_std(ctrl_handler, ops, V4L2_CID_ANALOGUE_GAIN, 
						AR0144_MIN_ANALOG_GAIN, AR0144_MAX_ANALOG_GAIN, STEP_VALUE_1, state->timings.analog_gain);
	if (ctrl_handler->error) {
		dev_err(&client->dev, "analog_gain v4l2_ctrl_new_std failed: %#010X\n", ctrl_handler->error);
		goto ctrl_error;
	}

	/* orientation is front user facing */
	ctrls->orientation = v4l2_ctrl_new_std_menu(ctrl_handler, ops, V4L2_CID_CAMERA_ORIENTATION, 
						V4L2_CAMERA_ORIENTATION_EXTERNAL, 0, V4L2_CAMERA_ORIENTATION_FRONT);
	if (ctrl_handler->error) {
		dev_err(&client->dev, "orientation v4l2_ctrl_new_std failed: %#010X\n", ctrl_handler->error);
		goto ctrl_error;
	}

	/* test pattern */
	ctrls->test_pattern = v4l2_ctrl_new_std_menu_items(ctrl_handler, ops, V4L2_CID_TEST_PATTERN,
					ARRAY_SIZE(test_pattern_menu) - 1, 0, 0, test_pattern_menu);
	if (ctrl_handler->error) {
		dev_err(&client->dev, "test_pattern v4l2_ctrl_new_std failed: %#010X\n", ctrl_handler->error);
		goto ctrl_error;
	}
	
	// parse the device tree to fetch the sensor properties
	ret = v4l2_fwnode_device_parse(&client->dev, &props);
	if (ret)
		goto ctrl_error;

	ret = v4l2_ctrl_new_fwnode_properties(ctrl_handler, ops, &props);
	if (ret)
		goto ctrl_error;
	
	ctrls->pixel_rate->flags |= V4L2_CTRL_FLAG_READ_ONLY;
	ctrls->hblank->flags |= V4L2_CTRL_FLAG_VOLATILE;
	ctrls->vblank->flags |= V4L2_CTRL_FLAG_VOLATILE;
	ctrls->exposure->flags |= V4L2_CTRL_FLAG_VOLATILE;
	ctrls->analog_gain->flags |= V4L2_CTRL_FLAG_READ_ONLY;
	ctrls->orientation->flags |= V4L2_CTRL_FLAG_READ_ONLY;

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

	dev_info(&client->dev, "Probing driver\n");
	dev_info(&client->dev, "I2C address: %#04X\n", client->addr);

	state = devm_kzalloc(&client->dev, sizeof(*state), GFP_KERNEL);
	if (!state) {
		return -ENOMEM;
	}

	// setup the regmap for the sensor
	state->regs = devm_regmap_init_i2c(client, &ar0144_regmap_config);
	if (IS_ERR(state->regs)) {
		ret = PTR_ERR(state->regs);
		dev_err(&client->dev, "Probe unable to init regmap: %#010X\n", ret);
		goto exit_probe;
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

	dev_info(&client->dev, "Initialised state\n");

	state->pad.flags = MEDIA_PAD_FL_SOURCE;
	ret = media_entity_pads_init(&state->sd.entity, 1, &state->pad);
	if (ret) {
		dev_err(&client->dev, "Probe unable to init media pads: %#010X\n", ret);
		goto exit_probe;
	}

	ret = ar0144_init_timings(state);
	if (ret < 0) {
		dev_err(&client->dev, "Probe unable to init timings: %#010X\n", ret);
		goto exit_probe;
	}

	ret = ar0144_init_controls(state);
	if (ret < 0) {
		dev_err(&client->dev, "Probe unable to init controls: %#010X\n", ret);
		goto exit_probe;
	}

	// set the format details
	ret = ar0144_set_fmt_to_defaults(state);
	if (ret < 0) {
		dev_err(&client->dev, "Probe unable to set default formats: %#010X\n", ret);
		goto exit_probe;
	}

	ret = v4l2_async_register_subdev_sensor(&state->sd);
	if (ret < 0) {
		dev_err(&client->dev, "Probe unable to resister async: %#010X\n", ret);
		goto exit_probe;
	}

	// power on the sensor
	ret = ar0144_subdev_s_power(&state->sd, 1);
	if (ret < 0) {
		dev_err(&client->dev, "Probe unable to power on sensor: %#010X\n", ret);
		goto exit_probe;
	}

	dev_info(&client->dev, "Probe successful\n");
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

	if (sd) {
		ar0144_subdev_s_power(sd, 0);
		v4l2_async_unregister_subdev(sd);
	}

	mutex_destroy(&state->lock);
	dev_info(&client->dev, "Removed driver\n");
}

/* create the init and exit functions */
module_i2c_driver(ar0144_i2c_driver);

MODULE_AUTHOR("Christopher Storah <christopher.storah@leicabiosystems.com>");
MODULE_ALIAS("i2c:ar0144");
MODULE_DESCRIPTION("ON Semiconductor AR0144 Camera Sensor");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("0.1");
