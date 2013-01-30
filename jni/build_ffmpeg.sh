#!/bin/bash
######################################################
# Usage:
# put this script in top of FFmpeg source tree
# ./build_android
# It generates binary for following architectures:
# ARMv6 
# ARMv6+VFP 
# ARMv7+VFPv3-d16 (Tegra2) 
# ARMv7+Neon (Cortex-A8)
# Customizing:
# 1. Feel free to change ./configure parameters for more features
# 2. To adapt other ARM variants
# set $CPU and $OPTIMIZE_CFLAGS 
# call build_one
######################################################

set -e

NDK=~/android-ndk
ARM_PLATFORM=$NDK/platforms/android-5/arch-arm/
ARM_PREBUILT=$NDK/toolchains/arm-linux-androideabi-4.6/prebuilt/darwin-x86
X86_PLATFORM=$NDK/platforms/android-9/arch-x86/
X86_PREBUILT=$NDK/toolchains/x86-4.6/prebuilt/darwin-x86
function build_one
{
if [ $ARCH == "arm" ] 
then
    PLATFORM=$ARM_PLATFORM
    PREBUILT=$ARM_PREBUILT
    HOST=arm-linux-androideabi
else
    PLATFORM=$X86_PLATFORM
    PREBUILT=$X86_PREBUILT
    HOST=i686-linux-android
fi

pushd ffmpeg
./configure --target-os=linux \
    --prefix=$PREFIX \
    --enable-cross-compile \
    --extra-libs="-lgcc" \
    --arch=$ARCH \
    --cc=$PREBUILT/bin/$HOST-gcc \
    --cross-prefix=$PREBUILT/bin/$HOST- \
    --nm=$PREBUILT/bin/$HOST-nm \
    --sysroot=$PLATFORM \
    --extra-cflags="-fvisibility=hidden -fdata-sections -ffunction-sections -Os -fpic -DANDROID -DHAVE_SYS_UIO_H=1 -Dipv6mr_interface=ipv6mr_ifindex -fasm -Wno-psabi -fno-short-enums -fno-strict-aliasing -finline-limit=300 $OPTIMIZE_CFLAGS " \
    --disable-shared \
    --enable-static \
    --enable-small \
    --extra-ldflags="-Wl,-rpath-link=$PLATFORM/usr/lib -L$PLATFORM/usr/lib -nostdlib -lc -lm -ldl -llog" \
    --disable-everything \
    --enable-decoder=h264 \
    --enable-demuxer=h264 \
    --enable-parser=h264 \
    --disable-ffplay \
    --disable-ffmpeg \
    --disable-ffprobe \
    --disable-network \
    --disable-avfilter \
    --disable-avdevice \
#    --enable-demuxer=mov \
#    --enable-demuxer=h264 \
#    --enable-protocol=file \
#    --enable-avformat \
#    --enable-avcodec \
#    --enable-decoder=rawvideo \
#    --enable-decoder=mjpeg \
#    --enable-decoder=h263 \
#    --enable-decoder=mpeg4 \
#    --enable-decoder=h264 \
#    --enable-parser=h264 \
#    --enable-zlib \
    $ADDITIONAL_CONFIGURE_FLAG

make clean
make  -j4 install V=1
$PREBUILT/bin/$HOST-ar d libavcodec/libavcodec.a inverse.o
#$PREBUILT/bin/$HOST-ld -rpath-link=$PLATFORM/usr/lib -L$PLATFORM/usr/lib  -soname libffmpeg.so -shared -nostdlib  -z,noexecstack -Bsymbolic --whole-archive --no-undefined -o $PREFIX/libffmpeg.so libavcodec/libavcodec.a libavformat/libavformat.a libavutil/libavutil.a libswscale/libswscale.a -lc -lm -lz -ldl -llog  --warn-once  --dynamic-linker=/system/bin/linker $PREBUILT/lib/gcc/$HOST/4.6/libgcc.a
popd
}

#arm v5te
CPU=armv5te
ARCH=arm
OPTIMIZE_CFLAGS="-marm -march=$CPU"
PREFIX=`pwd`/ffmpeg-android/$CPU 
ADDITIONAL_CONFIGURE_FLAG=
build_one

#arm v6
#CPU=armv6
#ARCH=arm
#OPTIMIZE_CFLAGS="-marm -march=$CPU"
#PREFIX=`pwd`/ffmpeg-android/$CPU 
#ADDITIONAL_CONFIGURE_FLAG=
#build_one

#arm v7vfpv3
CPU=armv7-a
ARCH=arm
OPTIMIZE_CFLAGS="-mfloat-abi=softfp -mfpu=vfpv3-d16 -marm -march=$CPU "
PREFIX=`pwd`/ffmpeg-android/$CPU
ADDITIONAL_CONFIGURE_FLAG=
build_one

#arm v7vfp
#CPU=armv7-a
#ARCH=arm
#OPTIMIZE_CFLAGS="-mfloat-abi=softfp -mfpu=vfp -marm -march=$CPU "
#PREFIX=`pwd`/ffmpeg-android/$CPU-vfp
#ADDITIONAL_CONFIGURE_FLAG=
#build_one

#arm v7n
#CPU=armv7-a
#ARCH=arm
#OPTIMIZE_CFLAGS="-mfloat-abi=softfp -mfpu=neon -marm -march=$CPU -mtune=cortex-a8"
#PREFIX=`pwd`/ffmpeg-android/$CPU 
#ADDITIONAL_CONFIGURE_FLAG=--enable-neon
#build_one

#arm v6+vfp
#CPU=armv6
#ARCH=arm
#OPTIMIZE_CFLAGS="-DCMP_HAVE_VFP -mfloat-abi=softfp -mfpu=vfp -marm -march=$CPU"
#PREFIX=`pwd`/ffmpeg-android/${CPU}_vfp 
#ADDITIONAL_CONFIGURE_FLAG=
#build_one

#x86
CPU=i686
ARCH=i686
OPTIMIZE_CFLAGS="-fomit-frame-pointer"
PREFIX=`pwd`/ffmpeg-android/${CPU} 
ADDITIONAL_CONFIGURE_FLAG=
build_one
