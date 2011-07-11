cmd_arch/arm/boot/Image := /root/CodeSourcery/Sourcery_G++_Lite/bin/arm-none-eabi-objcopy -O binary -R .note -R .note.gnu.build-id -R .comment -S  vmlinux arch/arm/boot/Image
