// SPDX-License-Identifier: GPL-2.0
/*
 * A driver for the AR1335 camera
 */

#define DEBUG 1
#define CONFIG_REGMAP 1

#include "ar1335-camera-driver.h"
#include <linux/miscdevice.h>

#define DRIVER_NAME "ar1335-camera-driver"
#define DRIVER_VERSION  "0.0.1"

// on-semi AR1335 sensor as used by the DA4200-30mci
// Bayer image supported in RAW10, RAW8 and RAW6 formats.

static const struct regmap_config ar1335_sensor_regmap_config = {
        .reg_bits = 16,
        .val_bits = 16,
        .cache_type = REGCACHE_RBTREE,
};

//static const struct file_operations ar1335_fileops = {
//        .owner = THIS_MODULE,
////        .open = ar1335_open,
//        .unlocked_ioctl = ar1335_ioctl,
////        .release = ar1335_release,
//};

static struct of_device_id ar1335_of_match[] = {
        { .compatible = "camera,ar1335", },
        { },
};

static int ar1335_power_on(struct ar1335_power_rail *power_rail)
{
    // power on sequence as defined in table 21 of the AR1335 datasheet
    int err;
    struct ar1335_info *info = container_of(power_rail, struct ar1335_info, power);

//    if (unlikely(WARN_ON(!power_rail || !power_rail->iovdd || !power_rail->avdd))) {
    if (unlikely(WARN_ON(!power_rail || !power_rail->vdd))) {
        dev_err(&info->i2c_client->dev, "Power on: power rail not defined\n");
        return -EFAULT;
    }

    gpio_set_value(info->platform_data->reset_gpio, 0);
    gpio_set_value(info->platform_data->af_gpio, 1);
    gpio_set_value(info->platform_data->cam1_gpio, 0);
    usleep_range(10, 20);

    dev_info(&info->i2c_client->dev, "Enabling Vdd regulator\n");
    err = regulator_enable(power_rail->vdd);
    if (err) {
        dev_err(&info->i2c_client->dev, "Failed to enable Vdd regulator\n");
        return -ENODEV;
    }

//    dev_info(&info->i2c_client->dev, "Enabling analog Vdd regulator\n");
//    err = regulator_enable(power_rail->avdd);
//    if (err) {
//        dev_err(&info->i2c_client->dev, "Failed to enable analog Vdd regulator\n");
//        return -ENODEV;
//    }
//
//    dev_info(&info->i2c_client->dev, "Enabling IO Vdd regulator\n");
//    err = regulator_enable(power_rail->iovdd);
//    if (err) {
//        dev_err(&info->i2c_client->dev, "Failed to enable IO Vdd regulator\n");
//        regulator_disable(power_rail->avdd);
//        return -ENODEV;
//    }

    dev_info(&info->i2c_client->dev, "Resetting and enabling camera\n");
    usleep_range(1, 2);
    gpio_set_value(info->platform_data->reset_gpio, 1);
    gpio_set_value(info->platform_data->cam1_gpio, 1);
    usleep_range(300, 310);

    return 1;
}

static int ar1335_power_off(struct ar1335_power_rail *power_rail)
{
    // power off sequence as defined in table 20 of the AR1335 datasheet
    struct ar1335_info *info = container_of(power_rail, struct ar1335_info, power);

//    if (unlikely(WARN_ON(!power_rail || !power_rail->iovdd || !power_rail->avdd))) {
    if (unlikely(WARN_ON(!power_rail || !power_rail->vdd))) {
        dev_err(&info->i2c_client->dev, "Power off: power rail not defined\n");
        return -EFAULT;
    }

    // sleep for 1-2 uSec (precise delay)
    usleep_range(1, 2);
    gpio_set_value(info->platform_data->cam1_gpio, 0);
    usleep_range(1, 2);

//    regulator_disable(power_rail->iovdd);
//    regulator_disable(power_rail->avdd);
    regulator_disable(power_rail->vdd);

    return 0;
}

static struct ar1335_platform_data *ar1335_populate_platform_data_from_device_tree(struct i2c_client *client)
{
    struct device_node *np = client->dev.of_node;
    struct ar1335_platform_data *platform_data;
    const struct of_device_id *match;

    match = of_match_device(ar1335_of_match, &client->dev);
    if (!match) {
        dev_err(&client->dev, "Failed to find matching dt id\n");
        return NULL;
    }

    platform_data = devm_kzalloc(&client->dev, sizeof(*platform_data), GFP_KERNEL);
    if (!platform_data) {
        dev_err(&client->dev, "Failed to allocate platform data\n");
        return NULL;
    }

//    these are not defined in the DTS - see ov5640 camera "reset-gpios = <&gpio1 6 GPIO_ACTIVE_LOW>" and powerdown-gpios
    platform_data->cam1_gpio = of_get_named_gpio(np, "cam1-gpios", 0);
    platform_data->reset_gpio = of_get_named_gpio(np, "reset-gpios", 0);
    platform_data->af_gpio = of_get_named_gpio(np, "af-gpios", 0);

    platform_data->power_on = ar1335_power_on;
    platform_data->power_off = ar1335_power_off;

    return platform_data;
}

static int ar1335_regulator_get(struct ar1335_info *info,
                                struct regulator **vreg, char vreg_name[])
{
    struct regulator *reg = NULL;
    int err = 0;

    reg = regulator_get(&info->i2c_client->dev, vreg_name);
    if (unlikely(IS_ERR(reg))) {
        dev_err(&info->i2c_client->dev, "%s %s ERR: %d\n", __func__, vreg_name, (int)reg);
        err = PTR_ERR(reg);
        reg = NULL;
    } else
        dev_dbg(&info->i2c_client->dev, "%s: %s\n",
                __func__, vreg_name);

    *vreg = reg;
    return err;
}

static int ar1335_power_get(struct ar1335_info *info)
{
    struct ar1335_power_rail *pw = &info->power;
    int err = 0;

//    err |= ar1335_regulator_get(info, &pw->avdd, "vana"); /* analog 2.7v */
//    err |= ar1335_regulator_get(info, &pw->dvdd, "vdig_csi"); /* dig 1.2v */
//    err |= ar1335_regulator_get(info, &pw->iovdd, "vif"); /* IO 1.8v */
    err |= ar1335_regulator_get(info, &pw->vdd, "vdd"); /* IO 1.8v */

    return err;
}

static struct media_entity_operations ar1335_subdev_media_ops = {
    .get_fwnode_pad = 0,
    .link_setup = 0,
    .has_pad_interdep = 0,
    .link_validate = 0
};

static int probe_camera(struct i2c_client *client) {
    int ret;
    struct ar1335_info *camera_data;

    // allocate memory to store the metadata for the camera
    dev_info(&client->dev, "Allocating driver structure");
    camera_data = devm_kzalloc(&client->dev, sizeof(*camera_data), GFP_KERNEL);
    if (IS_ERR(camera_data)) {
        dev_err(&client->dev, "Failed to allocate memory for driver structure: %ld\n", PTR_ERR(camera_data));
        return -ENOMEM;
    }

    // map the I2C registers for the sensor (memory is automatically freed on driver unload)
    dev_info(&client->dev, "Initialising I2C register map");
    camera_data->regmap = devm_regmap_init_i2c(client, &ar1335_sensor_regmap_config);
    if (IS_ERR(camera_data->regmap)) {
        dev_err(&client->dev,
                "regmap init failed: %ld\n", PTR_ERR(camera_data->regmap));
        return -ENODEV;
    }

    // populate the platform data
    dev_info(&client->dev, "Populating platform data from device tree");
    camera_data->platform_data = ar1335_populate_platform_data_from_device_tree(client);
    if (IS_ERR(camera_data->platform_data)) {
        dev_err(&client->dev,
                "memory allocation failed for platform data: %ld\n", PTR_ERR(camera_data->platform_data));
        return -ENODEV;
    }

    camera_data->i2c_client = client;

    // set the private data to the camera data structure
    dev_set_drvdata(&client->dev, &camera_data);

    // fetch the regulator information and store in the device
    ar1335_power_get(camera_data);

    // create a device node during registering the device and mark the media pad as a source of data (camera image)
    camera_data->subdev.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
    camera_data->pad.flags = MEDIA_PAD_FL_SOURCE;
    camera_data->subdev.entity.function = MEDIA_ENT_F_CAM_SENSOR;
    camera_data->subdev.entity.ops = &ar1335_subdev_media_ops;

    ret = media_entity_pads_init(&camera_data->subdev.entity, 1, &camera_data->pad);
    if (ret) {
        dev_err(&client->dev, "Initialise media entity pads failed\n");
        return -ENODEV;
    }

    // TODO: https://www.kernel.org/doc/html/v4.15/media/kapi/v4l2-device.html#c.v4l2_device_register_subdev_nodes
    ret = v4l2_async_register_subdev(&camera_data->subdev);
    if (ret) {
        dev_err(&client->dev, "V4L2 async register subdev failed\n");
        return -ENODEV;
    }

    dev_err(&client->dev, "%s driver probed\n", DRIVER_NAME);
    dev_err(&client->dev, "LBS Camera Driver v" DRIVER_VERSION);
    return 0;
}

static void remove_driver(struct i2c_client *client) {
    void* camera_data = client->dev.driver_data;
    if (camera_data) {
        // free our metadata
        devm_kfree(&client->dev, camera_data);
    }
}

static void shutdown(struct i2c_client *client) {
    // system is shutting down
}

static void alert(struct i2c_client *client, enum i2c_alert_protocol protocol, unsigned int data) {

}

static int detect(struct i2c_client *client, struct i2c_board_info *info) {
    dev_info(&client->dev, "detect - not implemented yet");
    return 0;
}

static int command(struct i2c_client *client, unsigned int cmd, void *arg) {
    int err = 0;
    struct ar1335_info *camera_data = dev_get_drvdata(&client->dev);

    switch(cmd) {
            case AR1335_IOCTL_SET_POWER:
                dev_info(&client->dev, "AR1335_IOCTL_SET_POWER - setting power");
                if (!camera_data->platform_data)
                    break;
                if (arg && camera_data->platform_data->power_on) {
//                err = ar1335_mclk_enable(info);
//                    if (!err) {
                        dev_info(&client->dev, "power on");
                        err = camera_data->platform_data->power_on(&camera_data->power);
//                    }
                    if (err < 0) {
                        dev_err(&client->dev, "unable to power on");
//                    ar1335_mclk_disable(info);
                    }
                }
                if (!arg && camera_data->platform_data->power_off) {
                    dev_info(&client->dev, "power off");
                    camera_data->platform_data->power_off(&camera_data->power);
//                ar1335_mclk_disable(info);
                }
                break;
        };
    return 0;
}

static struct i2c_device_id ar1335_camera_device_id_table[] = {
        { "ar1335-camera", 0 },
        { }
};

MODULE_DEVICE_TABLE(i2c, ar1335_camera_device_id_table);

// power management handling
struct dev_pm_ops ar1335_camera_pm_ops = {
    // TODO: suspend and resume may be required at some point
};

struct i2c_driver camera_driver = {
    .class = I2C_CLASS_HWMON,
    .driver = {
        .name = DRIVER_NAME,
        .of_match_table = ar1335_of_match,
        .pm = &ar1335_camera_pm_ops,
    },
    .id_table = ar1335_camera_device_id_table,
    .probe = probe_camera,
    .remove = remove_driver,
    .shutdown = shutdown,
    .alert = alert,
    .command = command,
    .detect = detect,
    .flags = 0
};

module_i2c_driver(camera_driver);

MODULE_AUTHOR("Christopher Storah <chris.storah@gmail.com>");
MODULE_DESCRIPTION("Low-level driver for the AR1335 Camera");
MODULE_LICENSE("GPL v2");
