#include <linux/regmap.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/v4l2-subdev.h>
#include <media/v4l2-subdev.h>


#define AR1335_IOCTL_SET_MODE		_IOW('o', 1, struct ar1335_mode)
#define AR1335_IOCTL_GET_STATUS		_IOR('o', 2, __u8)
#define AR1335_IOCTL_SET_FRAME_LENGTH	_IOW('o', 3, __u32)
#define AR1335_IOCTL_SET_COARSE_TIME	_IOW('o', 4, __u32)
#define AR1335_IOCTL_SET_GAIN		_IOW('o', 5, __u16)
#define AR1335_IOCTL_GET_SENSORDATA	_IOR('o', 6, struct ar1335_sensordata)
#define AR1335_IOCTL_SET_GROUP_HOLD	_IOW('o', 7, struct ar1335_ae)
#define AR1335_IOCTL_SET_POWER		_IOW('o', 20, __u32)
#define AR1335_IOCTL_GET_FLASH_CAP	_IOR('o', 30, __u32)
#define AR1335_IOCTL_SET_FLASH_MODE	_IOW('o', 31, \
						struct ar1335_flash_control)

struct ar1335_power_rail {
    struct regulator *vdd;/* 5V */
//    struct regulator *dvdd;
//    struct regulator *avdd;
//    struct regulator *iovdd;
};

struct ar1335_platform_data {
//    struct ar1335_flash_control flash_cap;
//    const char *mclk_name; /* NULL for default default_mclk */
//    bool ext_reg;
    unsigned int cam1_gpio;
    unsigned int reset_gpio;
    unsigned int af_gpio;
    int (*power_on)(struct ar1335_power_rail *pw);
    int (*power_off)(struct ar1335_power_rail *pw);
};


struct ar1335_info {
    struct i2c_client               *i2c_client;
    struct v4l2_subdev              subdev;
    struct media_pad                pad;
    struct ar1335_power_rail    	power;
    struct ar1335_platform_data	    *platform_data;
    struct regmap			        *regmap;

//    struct ar1335_sensordata	sensor_data;
//    struct clk			*mclk;
//    struct miscdevice		miscdev_info;
//    int				mode;
//    struct dentry			*debugdir;
//    atomic_t			in_use;
};
