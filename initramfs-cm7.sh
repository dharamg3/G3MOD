cd initramfs-cm7
find . -print0 | cpio --null -ov --format=newc > ../initramfs.cpio
