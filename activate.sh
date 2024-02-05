#!/bin/bash
OEROOT=$PWD/sources/openembedded-core
echo "OEROOT = $OEROOT"
. $OEROOT/oe-init-build-env $PWD/build
