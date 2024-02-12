# Cross compile the kernel source (outside of Yocto) for development
export ARCH=arm64
export CROSS_COMPILE=aarch64-linux-gnu-
LINUX_SOURCE=$(cd ~/linux-fslc_6.6 && pwd -P)
FILES_PATH=$(cd `dirname $0` && pwd -P)/linux-fslc/files
pushd $LINUX_SOURCE
# to add new default config settings for a later kernel, run the following commands and see .config
cp $FILES_PATH/kernel-6-6-defconfig $LINUX_SOURCE/.config

# apply the patches
git apply $FILES_PATH/0001-device-tree-supporting-ov5640.patch
git apply $FILES_PATH/0002-added-ar0144-dts.patch
git apply $FILES_PATH/0003-added-poc-dtb-into-makefile.patch
git apply $FILES_PATH/0006-added-regulators-to-hdmi-adv7535.patch

# ar0144 driver
git apply $FILES_PATH/0004-added-ar0144-driver.patch
git apply $FILES_PATH/0005-added-ar0144-into-makefile.patch
git apply $FILES_PATH/0007-added-controls-for-libcamera-to-ar0144.patch
popd