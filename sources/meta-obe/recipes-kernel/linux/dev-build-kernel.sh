# Cross compile the kernel source (outside of Yocto) for development
export ARCH=arm64
export CROSS_COMPILE=aarch64-linux-gnu-
LINUX_SOURCE=$(cd ~/linux-fslc_6.6 && pwd -P)
FILES_PATH=$(cd `dirname $0` && pwd -P)/linux-fslc/files
pushd $LINUX_SOURCE

# see dev-patch-kernel.sh for source setup

# make olddefconfig

# build
make -j`nproc`
# make modules
#
# install modules
#export INSTALL_MOD_PATH=~/linux-$LINUX_VERSION/lib-modules-output
#make modules_install
popd