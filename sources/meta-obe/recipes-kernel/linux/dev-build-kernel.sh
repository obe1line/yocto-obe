# Cross compile the kernel source (outside of Yocto) for development
config=""
patch=""
build=""
install=""

while getopts p:d:c:b:i:x:z: flag
do
    case "${flag}" in
        p) patch=${OPTARG};;
        d) dev=${OPTARG};;
        c) config=${OPTARG};;
        b) build=${OPTARG};;
        i) install=${OPTARG};;
        x) clean=${OPTARG};;
        z) distclean=${OPTARG};;
    esac
done

export ARCH=arm64
export CROSS_COMPILE=aarch64-linux-gnu-

FILES_PATH=$(cd `dirname $0` && pwd -P)/linux-fslc/files

LINUX_SOURCE=$(cd ~/linux-fslc_6.6 && pwd -P)
pushd $LINUX_SOURCE

if [ -n "$patch" ]; then
  echo "# apply the patches"
  git apply $FILES_PATH/patches/0001-added-ar0144-dts.patch
  git apply $FILES_PATH/patches/0004-added-ar0144-driver.patch
  git apply $FILES_PATH/patches/0005-added-ar0144-into-makefile.patch
  git apply $FILES_PATH/patches/0007-added-ar0144-into-config.patch
fi

if [ -n "$dev" ]; then
  cp $FILES_PATH/dev/ar0144.c $LINUX_SOURCE/drivers/media/i2c/
fi

if [ -n "$distclean" ]; then
  make distclean
fi

if [ -n "$clean" ]; then
  make clean
fi

if [ -n "$config" ]; then
  echo "# copy config file"
  cp $FILES_PATH/config/kernel-6-6.cfg $LINUX_SOURCE/.config
  make oldconfig
fi

if [ -n "$build" ]; then
  echo "# build"
  make -j`nproc` LOCALVERSION=-obe
  make modules LOCALVERSION=-obe
fi

if [ -n "$install" ]; then
  echo "# install modules"
  export INSTALL_MOD_PATH=./lib-modules-output
  make modules_install
fi

popd