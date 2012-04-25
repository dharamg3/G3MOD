#!/sbin/busybox sh
# Copyright 2010 Renaud Allard. All rights reserved.
#
# Credits: Sebastien Dadier - s.dadier@gmail.com 
# Original Developer for This File in Fugumod for I5800
#
# Redistribution and use in source and binary forms, with or without modification, are
# permitted provided that the following conditions are met:
#
#    1. Redistributions of source code must retain the above copyright notice, this list of
#       conditions and the following disclaimer.
#
#    2. Redistributions in binary form must reproduce the above copyright notice, this list
#       of conditions and the following disclaimer in the documentation and/or other materials
#       provided with the distribution.
# 
# THIS SOFTWARE IS PROVIDED BY Renaud Allard ``AS IS'' AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
# FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL Renaud Allard OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# The views and conclusions contained in the software and documentation are those of the
# authors and should not be interpreted as representing official policies, either expressed
# or implied, of Renaud Allard.

G3DIR=/g3mod_sd/Android/data/g3mod

DATA2SD=

build_fs_current()
{
	echo 'DO NOT MODIFY THIS FILE, YOUR SYSTEM WOULD NOT BE ABLE TO BOOT' > $G3DIR/fs.current
	mkdir /g3_mnt
	for DEVICE in mmcblk0p2 stl6 stl7 stl8
	do
		mount -o ro /dev/block/${DEVICE} /g3_mnt
		mount | awk '/\/g3_mnt/ { print $1 " " $5 }' | awk -F "/" '{ print $4 }' | sed 's/vfat/rfs/' >> $G3DIR/fs.current
		umount /g3_mnt
	done
	rmdir /g3_mnt
}


set_mount_options()
{
case $STL6_FS in
	jfs)
	STL6_MNT=',noatime,iocharset=utf8'
	;;
	ext4)
	STL6_MNT=',noatime,data=writeback,noauto_da_alloc,barrier=0,commit=20'
	;;
	ext2)
	STL6_MNT=''
	;;
	ext3)
	STL6_MNT=''
	;;
	reiserfs)
	STL6_MNT=',noatime,notail'
	;;
	rfs)
	STL6_MNT=',noatime,check=no'
	;;
esac
case $STL7_FS in
	jfs)
	STL7_MNT=',noatime,iocharset=utf8'
	;;
	ext4)
	STL7_MNT=',noatime,data=ordered,noauto_da_alloc,barrier=0,commit=20'
	;;
	ext2)
	STL7_MNT=''
	;;
	ext3)
	STL7_MNT=''
	;;
	reiserfs)
	STL7_MNT=',noatime,notail'
	;;
	tmpfs)
	STL7_MNT=',size=200m'
	;;
	rfs)
	STL7_MNT=',noatime,check=no'
	;;
esac
case $STL8_FS in
	jfs)
	STL8_MNT=',noatime,iocharset=utf8'
	;;
	ext4)
	STL8_MNT=',noatime,data=writeback,noauto_da_alloc,barrier=0,commit=20'
	;;
	ext2)
	STL8_MNT=''
	;;
	ext3)
	STL8_MNT=''
	;;
	reiserfs)
	STL8_MNT=',noatime,notail'
	;;
	tmpfs)
	STL8_MNT=',size=32m'
	;;
	rfs)
	STL8_MNT=',noatime,check=no'
	;;
esac
case $MMC_FS in
	jfs)
	MMC_MNT=',noatime,iocharset=utf8'
	;;
	ext4)
	MMC_MNT=',noatime,data=ordered,noauto_da_alloc,barrier=1,commit=20'
	;;
	ext2)
	MMC_MNT=''
	;;
	ext3)
	MMC_MNT=''
	;;
	reiserfs)
	MMC_MNT=',noatime,notail'
	;;
	btrfs)
	MMC_MNT=',nodatacow,compress,ssd'
	;;
	tmpfs)
	MMC_MNT=',size=200m'
	;;
	rfs)
	MMC_MNT=',noatime,check=no'
	;;
esac
}

get_filesystems() {
	STL6_FS=`grep stl6 $G3DIR/fs.current | awk '{ print $2 }'`
	STL7_FS=`grep stl7 $G3DIR/fs.current | awk '{ print $2 }'`
	STL8_FS=`grep stl8 $G3DIR/fs.current | awk '{ print $2 }'`
	MMC_FS=`grep mmcblk0p2 $G3DIR/fs.current | awk '{ print $2 }'`
}

    export PATH=/sbin:/vendor/bin:/system/sbin:/system/bin:/system/xbin
    export LD_LIBRARY_PATH=/vendor/lib:/system/lib
    export ANDROID_BOOTLOGO=1
    export ANDROID_CACHE=/cache
    export ANDROID_ROOT=/system
    export ANDROID_ASSETS=/system/app
    export ANDROID_DATA=/data
    export DOWNLOAD_CACHE=/cache/download
    export EXTERNAL_STORAGE=/mnt/sdcard
    export ASEC_MOUNTPOINT=/mnt/asec
    export LOOP_MOUNTPOINT=/mnt/obb
    export SD_EXT_DIRECTORY=/sd-ext
    export BOOTCLASSPATH=/system/framework/core.jar:/system/framework/bouncycastle.jar:/system/framework/ext.jar:/system/framework/framework.jar:/system/framework/android.policy.jar:/system/framework/services.jar:/system/framework/core-junit.jar

busybox insmod -f /lib/modules/fsr.ko
busybox insmod -f /lib/modules/fsr_stl.ko
busybox insmod -f /lib/modules/rfs_glue.ko
busybox insmod -f /lib/modules/rfs_fat.ko
busybox insmod -f /lib/modules/tun.ko

mkdir /proc
mkdir /sys
mount -t proc proc /proc
mount -t sysfs sys /sys
mkdir /dev
mknod /dev/null c 1 3
mknod /dev/zero c 1 5
mknod /dev/urandom c 1 9
mkdir /dev/block
mknod /dev/block/mmcblk0 b 179 0
mknod /dev/block/mmcblk0p1 b 179 1
mknod /dev/block/mmcblk0p2 b 179 2
mknod /dev/block/mmcblk0p3 b 179 3
mknod /dev/block/mmcblk0p4 b 179 4
mknod /dev/block/stl1 b 138 1
mknod /dev/block/stl2 b 138 2
mknod /dev/block/stl3 b 138 3
mknod /dev/block/stl4 b 138 4
mknod /dev/block/stl5 b 138 5
mknod /dev/block/stl6 b 138 6
mknod /dev/block/stl7 b 138 7
mknod /dev/block/stl8 b 138 8
mknod /dev/block/stl9 b 138 9
mknod /dev/block/stl10 b 138 10
mknod /dev/block/stl11 b 138 11
mknod /dev/block/stl12 b 138 12
mknod /dev/block/bml0!c b 137 0
mknod /dev/block/bml1 b 137 1
mknod /dev/block/bml2 b 137 2
mknod /dev/block/bml3 b 137 3
mknod /dev/block/bml4 b 137 4
mknod /dev/block/bml5 b 137 5
mknod /dev/block/bml6 b 137 6
mknod /dev/block/bml7 b 137 7
mknod /dev/block/bml8 b 137 8
mknod /dev/block/bml9 b 137 9
mknod /dev/block/bml10 b 137 10
mknod /dev/block/bml11 b 137 11
mknod /dev/block/bml12 b 137 12

mkdir /cache
mkdir /data
mkdir /system
chmod 0777 /data
chmod 0777 /cache
chown system /data
chgrp system /data
chown system /cache
chgrp cache /cache

# Mounting sdcard on g3mod_sd
mkdir /g3mod_sd
mount -t vfat -o utf8 /dev/block/mmcblk0p1 /g3mod_sd

mkdir /sd-ext

# Check if conf directory exists, if not create one
if ! test -d $G3DIR
then

	rm -rf $G3DIR
	mkdir -p $G3DIR

	# Check if there is an old fs.current
	if test -f /g3mod_sd/g3mod/fs.current
	then
		mv /g3mod_sd/g3mod/fs.current $G3DIR/fs.current
	fi
	if test -f /g3mod_sd/g3mod/fs.convert
	then
		mv /g3mod_sd/g3mod/fs.convert $G3DIR/fs.convert
	fi
	if test -f /g3mod_sd/g3mod/fs.options
	then
		mv /g3mod_sd/g3mod/fs.options $G3DIR/fs.options
	fi
	rm -rf /g3mod_sd/g3mod

	sync

fi


# if mounting system fails this will allow adb to run
mkdir /system/bin

echo "Build fs_current"
build_fs_current

echo "fsck filesystems"
for DEVICE in stl7 stl8 stl6 mmcblk0p2 
do
	case `grep ${DEVICE} $G3DIR/fs.current | awk '{ print $2 }'` in
jfs)
	/sbin/jfs_fsck -p /dev/block/${DEVICE}
;;
ext*)
	/sbin/e2fsck -p /dev/block/${DEVICE}
;;
reiserfs)
	echo /sbin/reiserfsck /dev/block/${DEVICE}
;;
btrfs)
	/sbin/btrfsck /dev/block/${DEVICE}
;;
	esac
done

umount /sdcard /cache /system /data

#fsck filesystems
for DEVICE in mmcblk0p2 stl6 stl7 stl8
do
	echo "Checking ${DEVICE}" >>/g3mod.log
	case `grep ${DEVICE} $G3DIR/fs.current | awk '{ print $2 }'` in
	jfs)
	/sbin/jfs_fsck -p /dev/block/${DEVICE} >>/g3mod.log 2>>/g3mod.log
	;;
	ext*)
	/sbin/e2fsck -p /dev/block/${DEVICE} >>/g3mod.log 2>>/g3mod.log
	;;
	reiserfs)
	echo "reiserfs, not checking" >>/g3mod.log 2>>/g3mod.log
	;;
	btrfs)
	/sbin/btrfsck /dev/block/${DEVICE} >>/g3mod.log 2>>/g3mod.log
	;;
	rfs)
	echo "rfs, not checking" >>/g3mod.log 2>>/g3mod.log
	;;
	esac
done

rm /g3mod_sd/g3mod_data.tar

#check if we need to convert something (from sanitized output)
if test -f $G3DIR/fs.convert
then
	mkdir /g3mod_data
	egrep "stl|mmc" $G3DIR/fs.convert | tr '\011' ' ' | sed 's/ */ /g' | sed 's/^ *//g' | awk '{ print $1 " " $2 }' | while read DEVICE NEW_FS
	do
		CUR_FS=`grep ${DEVICE} $G3DIR/fs.current | awk '{ print $2 }'`
		if [ "${CUR_FS}" != "${NEW_FS}" ]
		then
			echo "$DEVICE is in $CUR_FS and needs to be converted to $NEW_FS" >> /g3mod.log
			case ${NEW_FS} in
			jfs)
			MKFS='/sbin/jfs_mkfs -q '
			;;
			ext4)
			MKFS='/sbin/mke2fs -J size=4 -T ext4 -b 4096 -O ^resize_inode,^ext_attr,^huge_file '
			;;
			ext3)
			MKFS='/sbin/mke2fs -J size=4 -T ext3 -b 4096 -O ^resize_inode,^ext_attr,^huge_file '
			;;
			ext2)
			MKFS='/sbin/mke2fs -T ext2 -b 4096 -O ^resize_inode,^ext_attr,^huge_file '
			;;
			reiserfs)
			MKFS='/sbin/mkreiserfs -q -b 4096 -s 1000 '
			;;
			btrfs)
			MKFS='/sbin/mkfs.btrfs -d single '
			;;
			rfs)
			MKFS='/sbin/fat.format '
			;;
			esac
			if [ "${CUR_FS}" == "rfs" ]
			then
				mount -t ${CUR_FS} -o ro,check=no /dev/block/${DEVICE} /g3mod_data 2>>/g3mod.log
			else
				mount -t ${CUR_FS} -o ro /dev/block/${DEVICE} /g3mod_data 2>>/g3mod.log
			fi
			echo "`date` : archiving ${DEVICE} for conversion" >>/g3mod.log
			mount |grep g3mod >>/g3mod.log
			cd /g3mod_data/
			tar cvf /g3mod_sd/g3mod_data.tar * >>/g3mod.log 2>>/g3mod.log
			cd /
			sync
			umount /dev/block/${DEVICE}
			echo "`date` : formatting ${DEVICE} for conversion" >>/g3mod.log			
			dd if=/dev/zero of=/dev/block/${DEVICE} bs=1024 count=10
			${MKFS} /dev/block/${DEVICE} >> /g3mod.log 2>> /g3mod.log
			if [ "${NEW_FS}" == "rfs" ]
			then
				mount -t ${NEW_FS} -o rw,noatime,nodiratime,check=no /dev/block/${DEVICE} /g3mod_data
			else
				mount -t ${NEW_FS} -o rw,noatime,nodiratime /dev/block/${DEVICE} /g3mod_data
			fi
			echo "`date` : unarchiving ${DEVICE}" >>/g3mod.log
			cd /g3mod_data/
			tar xvf /g3mod_sd/g3mod_data.tar >>/g3mod.log 2>>/g3mod.log
			cd /
			sync
			umount /dev/block/${DEVICE}
		fi
	done
	rm $G3DIR/fs.convert
	rmdir /g3mod_data
	#regenerate a fs.current
	build_fs_current
	rm /g3mod_sd/g3mod_data.tar
fi

if test -f $G3DIR/fs.tmpfs
then
	for DEVICE in mmcblk0p2 stl7 stl8 
	do
		sed -i "s/$DEVICE.*/$DEVICE tmpfs/" $G3DIR/fs.current
	done
	rm  /g3mod_sd/Android/data/g3mod/fs.tmpfs
fi

if test -f $G3DIR/android.logger
then
   insmod /lib/modules/logger.ko
fi

# get filesystems
get_filesystems

if test -f $G3DIR/fs.options
then
	STL6_MNT=",`grep stl6 $G3DIR/fs.options | awk '{ print $2 }'`"
	STL7_MNT=",`grep stl7 $G3DIR/fs.options | awk '{ print $2 }'`"
	STL8_MNT=",`grep stl8 $G3DIR/fs.options | awk '{ print $2 }'`"
	MMC_MNT=",`grep mmcblk0p2 $G3DIR/fs.options | awk '{ print $2 }'`"
else
	set_mount_options
fi
	
echo "STL6 :" $STL6_FS
echo "STL7 :" $STL7_FS
echo "STL8 :" $STL8_FS
echo "MMC : " $MMC_FS

rm -rf /etc
sync
cd /

mount -t $STL6_FS -o nodev,noatime,nodiratime,ro /dev/block/stl6 /system
mount -t $STL7_FS -o nodiratime,nosuid,nodev,rw$STL7_MNT /dev/block/stl7 /data

# DATA2SD CODE

if test -f $G3DIR/fs.data2sd; then
	DATA2SDmode=`cat $G3DIR/fs.data2sd`
	mkdir /sd-ext
	mount -t $MMC_FS -o rw,$MMC_MNT /dev/block/mmcblk0p2 /sd-ext


	if [ "$DATA2SDmode" = "hybrid" ]
	then
		echo "Data2SD Enabled - Hybrid Mode" >> /data2sd.log	
		echo "Data2SD Enabled - Hybrid Mode" >> /g3mod.log
		tr -d '\r' < /system/etc/data2sd.dirs > /data2sd.dirs
		tr -d '\r' < $G3DIR/data2sd.dirs > /data2sd.dirs
	else
		echo "Data2SD Enabled - Standard Mode" >> /data2sd.log	
		echo "Data2SD Enabled - Standard Mode" >> /g3mod.log
		umount /data
		mount -t $MMC_FS -o nodiratime,nosuid,nodev,rw$MMC_MNT /dev/block/mmcblk0p2 /sd-ext
	fi
	sed -i "s|g3_mount_stl7|# Line not needed for Data2SD|" /init_ics.rc /init_ging.rc /init_froyo.rc /init.rc /recovery.rc
else
	echo "Data2SD Disabled" >> /g3mod.log
	sed -i "s|g3_mount_stl7|# Already mounted in the pre-init.sh script|" /init_ics.rc /init_ging.rc /init_froyo.rc /init.rc /recovery.rc
fi

# modify mount options to inject in android inits
STL6_MNT=`echo ${STL6_MNT} | sed 's/\,/ /g'`
STL7_MNT=`echo ${STL7_MNT} | sed 's/\,/ /g'`
STL8_MNT=`echo ${STL8_MNT} | sed 's/\,/ /g'`

# END OF DATA2SD CODE

# Multi Data Code
if test -f $G3DIR/multiosdata; then
	MultiOS=`grep "ro.build.id" /system/build.prop|awk '{FS="="};{print $2}'`
	if test -f /sd-ext/lastos; then
		LastOS=`cat /sd-ext/lastos`
	fi
	mkdir /sd-ext/multios
	echo "Multi-OS Data Enabled: $MultiOS" >> /g3mod.log

	if test -f $G3DIR/multiosdata.comp; then	
		MultiOSCompression=`cat $G3DIR/multiosdata.comp`
	fi
else
	echo "Multi-OS Data Disabled" >> /g3mod.log
fi


echo "Cleaning up symlinks" >> /data2sd.log
cd /data/
for x in *
	do if [ -L $x ]; then
		echo "- /data/$x is a symlink" >> /data2sd.log
		rm /data/$x
		mkdir /data/$x
		chmod 777 /data/$x
	fi
done
mkdir /data/dalvik-cache
cd /data/dalvik-cache
for x in *
	do if [ -L $x ]; then
		echo "- /data/$x is a symlink" >> /data2sd.log
		rm /data/$x
		mkdir /data/$x
		chmod 777 /data/$x
	fi
done
cd /

if [ "$MultiOS" != "" ]; then
	if [ "$LastOS" = "" ]; then
		LastOS=$MultiOS
	fi

	if [ "$MultiOS" != "$LastOS" ]; then
		echo "System has changed! Multi-OS Data changing too..." >> /multidata.log
		rm /sd-ext/multios/$LastOS.data.tar
		rm /sd-ext/multios/$LastOS.dalvikcache.tar
		rm /sd-ext/multios/$LastOS.android_secure.tar

		if test -f $G3DIR/multiosdata.cache; then
			echo "Backing up dalvik-cache ($MultiOSCompression)" >> /multidata.log
			tar cvf$MultiOSCompression /sd-ext/multios/$LastOS.dalvikcache.tar /data/dalvik-cache 2>>/multidata.log
		fi
		rm -r /data/dalvik-cache/*

		echo "Backing up old data ($MultiOSCompression)" >> /multidata.log
		tar cvf$MultiOSCompression /sd-ext/multios/$LastOS.data.tar /data 2>>/multidata.log 
		rm -r /data/*

		if test -d /g3mod_sd/.android_secure; then
			echo "Backing up old android_secure ($MultiOSCompression)" >> /multidata.log
			tar cvf$MultiOSCompression /sd-ext/multios/$LastOS.android_secure.tar /g3mod_sd/.android_secure 2>>/multidata.log 
		fi
		rm -r /g3mod_sd/.android_secure/*
		
		echo "Extracting new data ($MultiOSCompression)" >> /multidata.log
		tar xvf$MultiOSCompression /sd-ext/multios/$MultiOS.data.tar 2>>/multidata.log
		if test -f $G3DIR/multiosdata.cache; then
			echo "Extracting new dalvik-cache ($MultiOSCompression)" >> /multidata.log
			tar xvf$MultiOSCompression /sd-ext/multios/$MultiOS.dalvikcache.tar 2>>/multidata.log
		fi
		if test -f /sd-ext/multios/$MultiOS.android_secure.tar; then
			echo "Extracting new android_secure ($MultiOSCompression)" >> /multidata.log
			tar xvf$MultiOSCompression /sd-ext/multios/$MultiOS.android_secure.tar 2>>/multidata.log 
		fi
		echo "Data switched from $LastOS to $MultiOS" >> /multidata.log
	fi		

	echo $MultiOS > /sd-ext/lastos
fi
# End of Multi Data

# Hybrid Data2SD Enabler
if test -f /data2sd.dirs; then
	echo "Connecting Hybrid Data2SD Links" >> /data2sd.log
	for line in $(grep 'app\|dalvik-cache\|data\|log' /data2sd.dirs)
	do
		DATA2SDtemp="$line"

		# We copy /data content the first time Hybrid Data2SD start
		cp -prf /data/$DATA2SDtemp /sd-ext/
		mkdir /sd-ext/$DATA2SDtemp

		# Remove an existing folder and symlink it with sd-ext
		rm -rf /data/$DATA2SDtemp
		ln -s /sd-ext/$DATA2SDtemp /data/$DATA2SDtemp
		echo "- /data/$DATA2SDtemp linked to /sd-ext/$DATA2SDtemp" >> /data2sd.log

		if [ "$line" == "dalvik-cache" ]; then
			if test -f $G3DIR/hybrid.intsys; then
				echo "Internalising dalvik-cache" >> /data2sd.log
				mkdir /data/dalvik-syscache
				cd /data/dalvik-cache/
				for x in system@*; do
					if [ -L $x ]; then
						echo "- /data/dalvik-cache/$x already internal" >> /data2sd.log
					else
						mv $x /data/dalvik-syscache/
						rm $x
						ln -s /data/dalvik-syscache/$x $x
						chmod 777 /data/dalvik-syscache/$x
						chmod 777 $x
						echo "- /data/dalvik-cache/$x internalised to /data/dalvik-syscache/$x" >> /data2sd.log
					fi
				done
				cd /
			fi
		fi
	done
	chmod 771 /sd-ext
else
	echo "No Data2SD config file found (/system/etc/data2sd.dirs or /sdcard/Android/data/g3mod/data2sd.dirs)" >> /data2sd.log
fi

rm /data2sd.dirs
rm /multios
cd /
# End of Hybrid Data2SD

# Identify CyanogenMod or Samsung
version=`grep 'ro.build.version.release' /system/build.prop | sed 's/ro\.build\.version\.release=\(.*\)/\1/' | sed 's/\(.*\)\../\1/'`
androidfinger=`grep "ro.build.id" /system/build.prop|awk '{FS="="};{print $2}'`

echo "System detected: $androidfinger" >> /g3mod.log
if [ $bootmode = "2" ]; then
	# Write FS in recovery.fstab. CWM5 needs them to work.
	sed -i "s|SYSTEM_FS|$STL6_FS|" /misc/recovery.fstab
	sed -i "s|DATA_FS|$STL7_FS|" /misc/recovery.fstab
	sed -i "s|CACHE_FS|$STL8_FS|" /misc/recovery.fstab
	sed -i "s|SDEXT_FS|$MMC_FS|" /misc/recovery.fstab

	# Not used. Just in case the init binary can't handle recovery.rc
	mv /init_ging.rc /init.rc

	INITbin=init_ging
	echo "System booted with Gingerbread recovery mode" >> /g3mod.log

elif [ "$version" == "2.3" ]; then
	mv /init_ging.rc /init.rc
	INITbin=init_ging

	echo "System booted with AOSP Gingerbread Kernel mode" >> /g3mod.log
elif [ "$version" == "4.0" ]; then
	mv /init_ics.rc /init.rc
	INITbin=init_ics

	echo "System booted with ICS Kernel mode" >> /g3mod.log
else
	mv /init_froyo.rc /init.rc
	INITbin=init_froyo

	if [ "$androidfinger" == "samsung_apollo/apollo/GT-I5800:2.2/FRF91/226611:user/release-keys" ]; then
		sed -i "s|g3_wifi_data_01|mkdir /data/misc/wifi 0777 wifi wifi|" /init.rc
		sed -i "s|g3_wifi_data_02|chown wifi wifi /data/misc/wifi|" /init.rc
		sed -i "s|g3_wifi_data_03|chmod 0777 /data/misc/wifi|" /init.rc
		sed -i "s|g3_wifi_data_04|mkdir /data/system 0775 system system|" /init.rc
		sed -i "s|g3_wifi_data_05|mkdir /data/system/wpa_supplicant 0777 wifi wifi|" /init.rc
		sed -i "s|g3_wifi_data_06|chown wifi wifi /data/system/wpa_supplicant|" /init.rc
		sed -i "s|g3_wifi_data_07|chmod 0777 /data/system/wpa_supplicant|" /init.rc
		sed -i "s|g3_wifi_service|service wpa_supplicant /system/bin/wpa_supplicant -Dwext -ieth0 -c/data/misc/wifi/wpa_supplicant.conf -dd|" /init.rc
		sed -i "s|g3_vibrator_module|vibrator|" /init.rc
		echo "STL7 mounting options: [nodiratime,nosuid,nodev,rw$STL7_MNT]" > g3mod.log

		echo "System booted with AOSP Froyo kernel mode" >> /g3mod.log
	else
		sed -i "s|g3_wifi_data_01|mkdir /data/wifi 0777 wifi wifi|" /init.rc
		sed -i "s|g3_wifi_data_02|mkdir /data/misc/wifi 0771 wifi wifi|" /init.rc
		sed -i "s|g3_wifi_data_03|chmod 0777 /data/misc/wifi/|" /init.rc
		sed -i "s|g3_wifi_data_04|# Line not needed for Samsung|" /init.rc
		sed -i "s|g3_wifi_data_05|# Line not needed for Samsung|" /init.rc
		sed -i "s|g3_wifi_data_06|# Line not needed for Samsung|" /init.rc
		sed -i "s|g3_wifi_data_07|# Line not needed for Samsung|" /init.rc
		sed -i "s|g3_wifi_service|service wpa_supplicant /system/bin/wpa_supplicant -Dwext -ieth0 -c/data/wifi/bcm_supp.conf|" /init.rc
		sed -i "s|g3_vibrator_module|vibrator-sam|" /init.rc
		sed -i "s|Si4709_driver.ko|Si4709_driver_sam.ko|" /init.rc
		echo "System booted with Samsung Froyo kernel mode" >> /g3mod.log
	fi
fi

# Select correct G3D module
if [ -f "/system/lib/egl/libGLES_fimg.so" ]; then
	sed -i "s|g3_g3d_module|s3c_g3d_openfimg|" /init.rc
	echo "G3D module : OpenFIMG" >> /g3mod.log
else
	sed -i "s|g3_g3d_module|s3c_g3d_samsung|" /init.rc
	echo "G3D module : Samsung" >> /g3mod.log
fi

rm /init_*.rc
rm /recovery_*.rc

umount /system

# Enable Compcache if enabled by user
if [ -e "$G3DIR/compcache" ]; then
	sed -i "s|g3_compcache_1|service ramzswap /sbin/ramzswap.sh|" /init.rc
	sed -i "s|g3_compcache_2|user root|" /init.rc
	sed -i "s|g3_compcache_3|oneshot|" /init.rc
	cp "$G3DIR/compcache" "/compcache"
	echo "Compcache enabled" >> /g3mod.log
else
	sed -i "s|g3_compcache_1|# Compcache disabled|" /init.rc
	sed -i "s|g3_compcache_2|# Compcache disabled|" /init.rc
	sed -i "s|g3_compcache_3|# Compcache disabled|" /init.rc
	echo "Compcache disabled" >> /g3mod.log

fi

# Inline inject mountpoints
sed -i "s|g3_mount_stl6|mount ${STL6_FS} /dev/block/stl6 /system nodev noatime nodiratime ro ${STL6_MNT}|" /init.rc
sed -i "s|g3_mount_stl6|mount ${STL6_FS} /dev/block/stl6 /system nodev noatime nodiratime rw ${STL6_MNT}|" /recovery.rc
sed -i "s|g3_mount_stl8|mount ${STL8_FS} /dev/block/stl8 /cache sync noexec noatime nodiratime nosuid nodev rw ${STL8_MNT}|" /init.rc /recovery.rc

if ! test -f $G3DIR/fs.data2sd; then
	umount /sd-ext
	rmdir /sd-ext
fi

umount /g3mod_sd
rmdir /g3mod_sd

ln -s ../$INITbin /sbin/ueventd
exec /$INITbin


