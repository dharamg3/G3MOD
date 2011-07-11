#!/bin/sh
make apollo_rev_02_android_defconfig
make -j8
cd modules/vibetonz
make KDIR=../../
cd ../fm_si4709
make KDIR=../../
