# Cross compile the kernel source (outside of Yocto) for development
export ARCH=arm64
export CROSS_COMPILE=aarch64-linux-gnu-
FILES_PATH=$(cd `dirname $0` && pwd -P)/files
cp $FILES_PATH/obe-defconfig ~/linux-6.4.16/.config
pushd ~/linux-6.4.16
# to add new default config settings for a later kernel, run the following command and see .config
# make olddefconfig
#
# apply the patches
# git apply $FILES_PATH/0001-updated-imx8mn-dts-from-nxp-repo.patch
# git apply $FILES_PATH/0002-imx8mn-ddr4-evk-Add-DTS-for-AP1302-ISP.patch
# git apply $FILES_PATH/0003-copied-top-dtsi-for-imx8mn-from-nxp-repo.patch
# git apply $FILES_PATH/0004-Add-camera-DTS-to-Makefile.patch
# git apply $FILES_PATH/0005-Added-voltage-regulators-to-adv7535.patch
# git apply $FILES_PATH/0006-Added-ap1302-driver-makefile-and-Kconfig.patch
#
# build
make
make modules
#
# install modules
export INSTALL_MOD_PATH=~/linux-6.4.16/lib-modules-output
make modules_install
popd