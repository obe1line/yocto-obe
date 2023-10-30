#!/bin/bash
OEROOT=$PWD/sources/poky
echo "OEROOT = $OEROOT"
. $OEROOT/oe-init-build-env $PWD/build
