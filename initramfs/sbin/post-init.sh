#!/sbin/busybox sh
# filesystem patcher
# Copyright SztupY, Licence: GPLv3
#
# Updated By: Mark Kennard - komcomputers@gmail.com
#
# G3MOD
#

export PATH=/sbin:/system/bin:/system/xbin
exec >>/tmp/postinit.log
exec 2>&1

chmod 777 /sdext/app

mkdir /system/sd
mount -o bind /sdext /system/sd

# continue with playing the logo
exec /system/bin/playlogo&

/sbin/busybox mount -o remount,rw /
if ! test -d /system/etc/init.d ; then
	mkdir /system/etc/init.d
fi

# init.d support
echo $(date) USER EARLY INIT START
if cd /system/etc/init.d >/dev/null 2>&1 ; then
    for file in E* ; do
        if ! cat "$file" >/dev/null 2>&1 ; then continue ; fi
        echo "START '$file'"
        /system/bin/sh "$file"
        echo "EXIT '$file' ($?)"
    done
fi
echo $(date) USER EARLY INIT DONE
echo $(date) USER INIT START
if cd /system/etc/init.d >/dev/null 2>&1 ; then
    for file in S* ; do
        if ! ls "$file" >/dev/null 2>&1 ; then continue ; fi
        echo "START '$file'"
        /system/bin/sh "$file"
        echo "EXIT '$file' ($?)"
    done
fi
echo $(date) USER INIT DONE

if test -f /system/bin/vold; then
	# rootfs and system should be closed for now
	/sbin/busybox mount -o remount,ro /system
	for x in /sbin/*
		do if [ -L $x ]; then
			rm /sbin/$x
		fi
	done

	# self destruct :)
	/sbin/busybox rm /sbin/busybox
else
	mkdir /system/bin
	ln -s /sbin/busybox /system/bin/sh
fi


