cd initramfs-froyo
find . -print0 | cpio --null -ov --format=newc > ../initramfs.cpio
