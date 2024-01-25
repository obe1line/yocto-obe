# Cross compile the kernel source (outside of Yocto) for development
export ARCH=arm64
export CROSS_COMPILE=aarch64-linux-gnu-
LINUX_VERSION="6.1.55"
FILES_PATH=$(cd `dirname $0` && pwd -P)/linux-fslc-imx/files
pushd ~/linux-$LINUX_VERSION
# to add new default config settings for a later kernel, run the following commands and see .config
# cp $FILES_PATH/obe6157-defconfig ~/linux-$LINUX_VERSION/.config
# make olddefconfig
#
# apply the patches
# git apply $FILES_PATH/0001-new-ap1302-driver.patch
# git apply $FILES_PATH/0002-imx8mn-ddr4-evk-Add-DTS-for-AP1302-ISP.patch
# git apply $FILES_PATH/0003-add-ap1302-dtb-into-makefile.patch
# git apply $FILES_PATH/0004-added-imx708-driver.patch
# git apply $FILES_PATH/0005-patch-media-type-header-for-imx708.patch
# git apply $FILES_PATH/0006-added-debug-to-ap1302.patch
# git apply $FILES_PATH/0007-added-ar0144-driver.patch
# git apply $FILES_PATH/0008-imx8mn-ddr4-evk-Add-DTS-for-AR0144.patch
# git apply $FILES_PATH/0009-added-ar0144-into-config.patch
#
# build
# make -j`nproc`
# make modules
#
# install modules
#export INSTALL_MOD_PATH=~/linux-$LINUX_VERSION/lib-modules-output
#make modules_install
popd