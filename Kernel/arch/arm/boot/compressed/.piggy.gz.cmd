cmd_arch/arm/boot/compressed/piggy.gz := (cat arch/arm/boot/compressed/../Image | gzip -f -9 > arch/arm/boot/compressed/piggy.gz) || (rm -f arch/arm/boot/compressed/piggy.gz ; false)
