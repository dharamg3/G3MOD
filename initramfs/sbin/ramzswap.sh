#!/sbin/busybox sh

insmod /lib/modules/ramzswap.ko

ccsize=`cat /compcache`
if [[ ! "$ccsize" == "" && ! "$ccsize" == "0" ]]; then
	ccsizekb=$(($ccsize*1024))
	/sbin/rzscontrol /dev/block/ramzswap0 --disksize_kb=$ccsizekb --init
	echo "Compcache started with $ccsize MB - $ccsizekb KB" >> /g3mod.log
else
	/sbin/rzscontrol /dev/block/ramzswap0 --init
	echo "Compcache started with default configuration" >> /g3mod.log
fi

swapon /dev/block/ramzswap0
rm /compcache
