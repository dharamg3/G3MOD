cd initramfs-cm6
find . -print0 | cpio --null -ov --format=newc > ../initramfs.cpio
