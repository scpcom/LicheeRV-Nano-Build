#!/bin/bash -e

export SG_BOARD_FAMILY=sg200x
export SG_BOARD_LINK=sg2002_licheervnano_sd

maixcdk=n
nanokvm=y
shrink=y
tailscale=n
tpudemo=n
tpusdk=n
while [ "$#" -gt 0 ]; do
	case "$1" in
	--board=*|--board-link=*)
		export SG_BOARD_LINK=`echo $1 | cut -d '=' -f 2-`
		shift
		;;
	--maix-cdk|--maixcdk)
		shift
		maixcdk=y
		;;
	--no-maix-cdk|--no-maixcdk)
		shift
		maixcdk=n
		;;
	--shrink)
		shift
		shrink=y
		;;
	--no-shrink)
		shift
		shrink=n
		;;
	--tailscale)
		shift
		tailscale=y
		;;
	--no-tailscale)
		shift
		tailscale=n
		;;
	--tpu-demo|--tpudemo)
		shift
		tpudemo=y
		;;
	--no-tpu-demo|--no-tpudemo)
		shift
		tpudemo=n
		;;
	--tpu-sdk|--tpusdk)
		shift
		tpusdk=y
		;;
	--no-tpu-sdk|--no-tpusdk)
		shift
		tpusdk=n
		;;
	*)
		break
		;;
	esac
done

for p in / /usr/ /usr/local/ ; do
  if echo $PATH | grep -q ${p}bin ; then
    if ! echo $PATH | grep -q ${p}sbin ; then
      export PATH=${p}sbin:$PATH
    fi
  fi
done

if echo ${SG_BOARD_LINK} | grep -q -E '^cv180' ; then
  export SG_BOARD_FAMILY=cv180x
fi
if echo ${SG_BOARD_LINK} | grep -q -E '^sg200' ; then
  export SG_BOARD_FAMILY=sg200x
fi

cd build
# Expand user space RAM from 128MB to 160MB
sed -i s/'ION_SIZE = .* . SIZE_1M'/'ION_SIZE = 75 * SIZE_1M'/g boards/${SG_BOARD_FAMILY}/${SG_BOARD_LINK}/memmap.py
sed -i s/'BOOTLOGO_SIZE = .* . SIZE_1K'/'BOOTLOGO_SIZE = 5632 * SIZE_1K'/g boards/${SG_BOARD_FAMILY}/${SG_BOARD_LINK}/memmap.py
# board config for maixcdk
if [ $maixcdk = y ]; then
  if ! grep -q "board" tools/common/sd_tools/genimage_rootless.cfg ; then
    sed -i s/'\t\t\t"usb.dev",'/'\t\t\t"usb.dev",\n\t\t\t"board",'/g tools/common/sd_tools/genimage_rootless.cfg
  fi
  if ! grep -q "board" tools/common/sd_tools/sd_gen_burn_image_rootless.sh ; then
    sed -i 's| \${output_dir}/input/usb.dev$| ${output_dir}/input/usb.dev\necho "id=maixcam" > ${output_dir}/input/board\necho "panel=st7701_hd228001c31" >> ${output_dir}/input/board|g' tools/common/sd_tools/sd_gen_burn_image_rootless.sh
  fi
fi
# enable usb disk, disable ncm
sed -i s/'usb.ncm'/'usb.disk0'/g tools/common/sd_tools/genimage_rootless.cfg
sed -i 's|touch ${output_dir}/input/usb.ncm|echo /dev/mmcblk0p3 > ${output_dir}/input/usb.disk0|g' tools/common/sd_tools/sd_gen_burn_image_rootless.sh
# enable usb hid
#if ! grep -q "usb.hid" tools/common/sd_tools/genimage_rootless.cfg ; then
#  sed -i s/'\t\t\t"usb.disk0",'/'\t\t\t"usb.disk0",\n\t\t\t"usb.hid",'/g tools/common/sd_tools/genimage_rootless.cfg
#fi
if ! grep -q "usb.touchpad" tools/common/sd_tools/genimage_rootless.cfg ; then
  sed -i s/'\t\t\t"usb.disk0",'/'\t\t\t"usb.disk0",\n\t\t\t"usb.touchpad",'/g tools/common/sd_tools/genimage_rootless.cfg
fi
if ! grep -q "usb.mouse" tools/common/sd_tools/genimage_rootless.cfg ; then
  sed -i s/'\t\t\t"usb.disk0",'/'\t\t\t"usb.disk0",\n\t\t\t"usb.mouse",'/g tools/common/sd_tools/genimage_rootless.cfg
fi
if ! grep -q "usb.keyboard" tools/common/sd_tools/genimage_rootless.cfg ; then
  sed -i s/'\t\t\t"usb.disk0",'/'\t\t\t"usb.disk0",\n\t\t\t"usb.keyboard",'/g tools/common/sd_tools/genimage_rootless.cfg
fi
#if ! grep -q "usb.hid" tools/common/sd_tools/sd_gen_burn_image_rootless.sh ; then
#  sed -i 's| \${output_dir}/input/usb.disk0$| ${output_dir}/input/usb.disk0\ntouch ${output_dir}/input/usb.hid|g' tools/common/sd_tools/sd_gen_burn_image_rootless.sh
#fi
if ! grep -q "usb.touchpad" tools/common/sd_tools/sd_gen_burn_image_rootless.sh ; then
  sed -i 's| \${output_dir}/input/usb.disk0$| ${output_dir}/input/usb.disk0\ntouch ${output_dir}/input/usb.touchpad|g' tools/common/sd_tools/sd_gen_burn_image_rootless.sh
fi
if ! grep -q "usb.mouse" tools/common/sd_tools/sd_gen_burn_image_rootless.sh ; then
  sed -i 's| \${output_dir}/input/usb.disk0$| ${output_dir}/input/usb.disk0\ntouch ${output_dir}/input/usb.mouse|g' tools/common/sd_tools/sd_gen_burn_image_rootless.sh
fi
if ! grep -q "usb.keyboard" tools/common/sd_tools/sd_gen_burn_image_rootless.sh ; then
  sed -i 's| \${output_dir}/input/usb.disk0$| ${output_dir}/input/usb.disk0\ntouch ${output_dir}/input/usb.keyboard|g' tools/common/sd_tools/sd_gen_burn_image_rootless.sh
fi
# set hostname prefix
if ! grep -q "hostname.prefix" tools/common/sd_tools/genimage_rootless.cfg ; then
  sed -i s/'\t\t\t"usb.touchpad",'/'\t\t\t"usb.touchpad",\n\t\t\t"hostname.prefix",'/g tools/common/sd_tools/genimage_rootless.cfg
fi
if ! grep -q "hostname.prefix" tools/common/sd_tools/sd_gen_burn_image_rootless.sh ; then
  sed -i 's| \${output_dir}/input/usb.touchpad$| ${output_dir}/input/usb.touchpad\necho -n kvm > ${output_dir}/input/hostname.prefix|g' tools/common/sd_tools/sd_gen_burn_image_rootless.sh
fi
cd ..

BR_OUTPUT_DIR=output

source build/cvisetup.sh
defconfig ${SG_BOARD_LINK}

cd buildroot
if [ -e ${BR_OUTPUT_DIR}/per-package/nanokvm-sg200x/target/kvmapp/system/init.d ]; then
  rsync -r --verbose --copy-dirlinks --copy-links --hard-links ${BR_OUTPUT_DIR}/per-package/nanokvm-sg200x/target/kvmapp/system/init.d/ board/cvitek/SG200X/overlay/etc/init.d/
  rm -f board/cvitek/SG200X/overlay/etc/init.d/S*kvm*
  rm -f board/cvitek/SG200X/overlay/etc/init.d/S*tailscale*
fi

if [ $maixcdk = y ]; then
  sed -i s/'^BR2_PACKAGE_PARTED=y'/'BR2_PACKAGE_MAIX_CDK=y\nBR2_PACKAGE_PARTED=y'/g configs/${BR_DEFCONFIG}
fi
if [ $maixcdk = y -a $shrink = y ]; then
  sed -i s/'^BR2_PACKAGE_MAIX_CDK=y'/'BR2_PACKAGE_MAIX_CDK=y\n# BR2_PACKAGE_MAIX_CDK_ALL_PROJECTS is not set'/g configs/${BR_DEFCONFIG}
  sed -i s/'^BR2_PACKAGE_MAIX_CDK=y'/'BR2_PACKAGE_MAIX_CDK=y\n# BR2_PACKAGE_MAIX_CDK_ALL_EXAMPLES is not set'/g configs/${BR_DEFCONFIG}
fi
if [ $nanokvm = y ]; then
  sed -i s/'^BR2_PACKAGE_PARTED=y'/'BR2_PACKAGE_NANOKVM_SG200X=y\nBR2_PACKAGE_PARTED=y'/g configs/${BR_DEFCONFIG}
fi
if [ $tailscale = y ]; then
  sed -i s/'^BR2_PACKAGE_PARTED=y'/'BR2_PACKAGE_TAILSCALE_RISCV64=y\nBR2_PACKAGE_PARTED=y'/g configs/${BR_DEFCONFIG}
fi
if [ $tpudemo = y ]; then
  sed -i s/'^# BR2_PACKAGE_TPUDEMO_SG200X is not set'/'BR2_PACKAGE_TPUDEMO_SG200X=y'/g configs/${BR_DEFCONFIG}
else
  sed -i s/'^BR2_PACKAGE_TPUDEMO_SG200X=y'/'# BR2_PACKAGE_TPUDEMO_SG200X is not set'/g configs/${BR_DEFCONFIG}
fi
if [ $shrink = y ]; then
  sed -i /'^BR2_PACKAGE_PYTHON_'/d configs/${BR_DEFCONFIG}

  sed -i s/'^BR2_PACKAGE_PYTHON3_PY_PYC=y$'/'BR2_PACKAGE_PYTHON3_PY_PYC=y'\
'\nBR2_PACKAGE_PYTHON_REQUESTS=y'\
'\nBR2_PACKAGE_PYTHON_REQUESTS_OAUTHLIB=y'\
'\nBR2_PACKAGE_PYTHON_REQUESTS_TOOLBELT=y'/g configs/${BR_DEFCONFIG}

  sed -i /'^BR2_PACKAGE_GDB'/d configs/${BR_DEFCONFIG}
  sed -i /'^BR2_PACKAGE_HOST_GDB'/d configs/${BR_DEFCONFIG}
  sed -i /'^BR2_GDB_'/d configs/${BR_DEFCONFIG}

  sed -i /'^BR2_PACKAGE_BLUEZ'/d configs/${BR_DEFCONFIG}
  sed -i /'^BR2_PACKAGE_LLDP'/d configs/${BR_DEFCONFIG}
  sed -i /'^BR2_PACKAGE_AVAHI'/d configs/${BR_DEFCONFIG}
  sed -i /'^BR2_PACKAGE_WIRELESS_TOOLS'/d configs/${BR_DEFCONFIG}
  sed -i /'^BR2_PACKAGE_AIRCRACK'/d configs/${BR_DEFCONFIG}
  sed -i /'^BR2_PACKAGE_UTIL_LINUX_RFKILL'/d configs/${BR_DEFCONFIG}
  sed -i /'^BR2_PACKAGE_IPMITOOL'/d configs/${BR_DEFCONFIG}
  sed -i /'^BR2_PACKAGE_SOCAT'/d configs/${BR_DEFCONFIG}
  sed -i /'^BR2_PACKAGE_TINC'/d configs/${BR_DEFCONFIG}
  sed -i /'^BR2_PACKAGE_WIRELESS_REGDB'/d configs/${BR_DEFCONFIG}
  sed -i /'^BR2_PACKAGE_MOSH'/d configs/${BR_DEFCONFIG}
  sed -i /'^BR2_PACKAGE_LRZSZ'/d configs/${BR_DEFCONFIG}
  sed -i /'^BR2_PACKAGE_MTR'/d configs/${BR_DEFCONFIG}
  sed -i /'^BR2_PACKAGE_WIREGUARD'/d configs/${BR_DEFCONFIG}
  sed -i /'^BR2_PACKAGE_WPA_SUPPLICANT'/d configs/${BR_DEFCONFIG}
  sed -i /'^BR2_PACKAGE_HOSTAPD'/d configs/${BR_DEFCONFIG}
  sed -i /'^BR2_PACKAGE_HAVEGED'/d configs/${BR_DEFCONFIG}
  sed -i /'^BR2_PACKAGE_DHRYSTONE'/d configs/${BR_DEFCONFIG}
  sed -i /'^BR2_PACKAGE_COREMARK'/d configs/${BR_DEFCONFIG}
  sed -i /'^BR2_PACKAGE_RAMSPEED'/d configs/${BR_DEFCONFIG}
  sed -i /'^BR2_PACKAGE_ALSA_UTILS'/d configs/${BR_DEFCONFIG}
  sed -i /'^BR2_PACKAGE_SQUASHFS'/d configs/${BR_DEFCONFIG}
  sed -i /'^BR2_PACKAGE_LCDTEST'/d configs/${BR_DEFCONFIG}
  sed -i /'^BR2_PACKAGE_ASCII_INVADERS'/d configs/${BR_DEFCONFIG}
  sed -i /'^BR2_PACKAGE_GNUCHESS'/d configs/${BR_DEFCONFIG}
  sed -i /'^BR2_PACKAGE_SL'/d configs/${BR_DEFCONFIG}
  sed -i /'^BR2_PACKAGE_XORCURSES'/d configs/${BR_DEFCONFIG}

  sed -i /'^BR2_PACKAGE_SSDP'/d configs/${BR_DEFCONFIG}
  sed -i /'^BR2_PACKAGE_PPPD'/d configs/${BR_DEFCONFIG}
  sed -i /'^BR2_PACKAGE_TCL'/d configs/${BR_DEFCONFIG}
fi
if [ $maixcdk = n -a $shrink = y ]; then
  sed -i /'^BR2_PACKAGE_FFMPEG'/d configs/${BR_DEFCONFIG}
  sed -i /'^BR2_PACKAGE_LIBQRENCODE'/d configs/${BR_DEFCONFIG}
  sed -i /'^BR2_PACKAGE_LIBWEBSOCKETS'/d configs/${BR_DEFCONFIG}
  sed -i /'^BR2_PACKAGE_MPG123'/d configs/${BR_DEFCONFIG}
  sed -i /'^BR2_PACKAGE_OPENCV'/d configs/${BR_DEFCONFIG}
fi

cd ..

if [ -e cviruntime -a -e flatbuffers ]; then
  # small fix to keep fork of flatbuffers repository optional
  sed -i s/'-Werror=unused-parameter"'/'-Werror=unused-parameter -Wno-class-memaccess"'/g flatbuffers/CMakeLists.txt
  [ $tpusdk = y ] && export TPU_REL=1
fi

build_all

cd build
git restore boards/${SG_BOARD_FAMILY}/${SG_BOARD_LINK}/memmap.py
git restore tools/common/sd_tools/genimage_rootless.cfg
git restore tools/common/sd_tools/sd_gen_burn_image_rootless.sh
cd ..

installdir=`pwd`/install/soc_${SG_BOARD_LINK}
cd buildroot
cd ${BR_OUTPUT_DIR}/target
if [ -e kvmapp/server/NanoKVM-Server ]; then
  rm -f ${installdir}/nanokvm-latest.zip
  ln -s kvmapp latest
  zip -r --symlinks ${installdir}/nanokvm-latest.zip latest/*
  rm latest
fi
cd -
rm -f board/cvitek/SG200X/overlay/etc/init.d/S*kvm*
rm -f board/cvitek/SG200X/overlay/etc/init.d/S*ssh*
rm -f board/cvitek/SG200X/overlay/etc/init.d/S*tailscale*
git restore board/cvitek/SG200X/overlay/etc/init.d
git restore configs/${BR_DEFCONFIG}
rm -f ${BR_OUTPUT_DIR}/target/etc/tailscale_disabled
rm -f ${BR_OUTPUT_DIR}/target/etc/init.d/S*kvm*
rm -f ${BR_OUTPUT_DIR}/target/etc/init.d/S*tailscale*
rm -f ${BR_OUTPUT_DIR}/target/usr/bin/tailscale
rm -f ${BR_OUTPUT_DIR}/target/usr/sbin/tailscaled
rm -rf ${BR_OUTPUT_DIR}/target/kvmapp/
cd ..

echo OK
