pushd sources
[ ! -d "openembedded-core" ] && git clone https://github.com/openembedded/openembedded-core --depth=1 --branch=nanbield
[ ! -d "bitbake" ] && git clone https://github.com/openembedded/bitbake --depth=1 --branch=2.6
[ ! -d "poky" ] && git clone https://git.yoctoproject.org/poky --depth=1 --branch=nanbield
[ ! -d "meta-arm" ] && git clone https://git.yoctoproject.org/meta-arm --depth=1 --branch=nanbield
[ ! -d "meta-virtualization" ] && git clone https://git.yoctoproject.org/meta-virtualization --depth=1 --branch=nanbield
[ ! -d "meta-openembedded" ] && git clone https://github.com/openembedded/meta-openembedded --depth=1 --branch=nanbield
[ ! -d "meta-freescale" ] && git clone https://github.com/Freescale/meta-freescale.git --depth=1 --branch=master
[ ! -d "meta-freescale-distro" ] && git clone https://github.com/Freescale/meta-freescale-distro.git --depth=1 --branch=master
popd

echo Now run ". ./activate.sh" to enable the build environment
echo To build a multimedia image run "bitbake imx-image-multimedia"
