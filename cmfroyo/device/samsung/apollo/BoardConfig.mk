USE_CAMERA_STUB := true

# inherit from the proprietary version
-include vendor/samsung/apollo/BoardConfigVendor.mk

TARGET_NO_BOOTLOADER := true
TARGET_BOARD_PLATFORM := unknown
TARGET_CPU_ABI := armeabi
TARGET_BOOTLOADER_BOARD_NAME := apollo

BOARD_KERNEL_CMDLINE := 
#BOARD_KERNEL_BASE := 0xe19f8000
#BOARD_PAGE_SIZE := 0x00000800

BOARD_USES_ALSA_AUDIO := true
BOARD_HAVE_BLUETOOTH := true
BT_USE_BTL_IF := true
BT_ALT_STACK := true
BRCM_BTL_INCLUDE_A2DP := true
BRCM_BT_USE_BTL_IF := true 

# fix this up by examining /proc/mtd on a running device
BOARD_BOOTIMAGE_PARTITION_SIZE := 0x00780000
BOARD_RECOVERYIMAGE_PARTITION_SIZE := 0x00700000
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 0x0d480000
BOARD_USERDATAIMAGE_PARTITION_SIZE := 0x0c380000
BOARD_FLASH_BLOCK_SIZE := 131072

TARGET_PREBUILT_KERNEL := device/samsung/apollo/kernel
#TARGET_PREBUILT_KERNEL :=



# custom recovery ui
BOARD_CUSTOM_RECOVERY_KEYMAPPING := ../../device/samsung/apollo/recovery/recovery_ui.c

# Below is a sample of how you can tweak the mount points using the board config.
# This is for the Samsung Galaxy S.
# Feel free to tweak or remove this code.
# If you want to add/tweak a mount point, the BOARD_X_FILESYSTEM_OPTIONS are optional.
#BOARD_DATA_DEVICE := /dev/block/mmcblk0p2
#BOARD_DATA_FILESYSTEM := auto
#BOARD_DATA_FILESYSTEM_OPTIONS := llw,check=no,nosuid,nodev
#BOARD_HAS_DATADATA := true
#BOARD_DATADATA_DEVICE := /dev/block/stl10
#BOARD_DATADATA_FILESYSTEM := auto
#BOARD_DATADATA_FILESYSTEM_OPTIONS := llw,check=no,nosuid,nodev
#BOARD_SYSTEM_DEVICE := /dev/block/stl9
#BOARD_SYSTEM_FILESYSTEM := auto
#BOARD_SYSTEM_FILESYSTEM_OPTIONS := llw,check=no
#BOARD_CACHE_DEVICE := /dev/block/stl11
#BOARD_CACHE_FILESYSTEM := auto
#BOARD_CACHE_FILESYSTEM_OPTIONS := llw,check=no,nosuid,nodev
#BOARD_SDEXT_DEVICE := /dev/block/mmcblk1p2


BOARD_BOOT_DEVICE := /dev/block/bml5

BOARD_DATA_DEVICE := /dev/block/stl7
BOARD_DATA_FILESYSTEM := ext4
#BOARD_DATA_FILESYSTEM_OPTIONS := llw,check=no,nosuid,nodev
BOARD_DATA_FILESYSTEM_OPTIONS := noatime,nodiratime,nobh,noauto_da_alloc,data=writeback,barrier=0,commit=20,nosuid,nodev
#BOARD_HAS_DATADATA := true
#BOARD_DATADATA_DEVICE := /dev/block/stl10
#BOARD_DATADATA_FILESYSTEM := auto
#BOARD_DATADATA_FILESYSTEM_OPTIONS := llw,check=no,nosuid,nodev
BOARD_SYSTEM_DEVICE := /dev/block/stl6
BOARD_SYSTEM_FILESYSTEM := ext2
#BOARD_SYSTEM_FILESYSTEM_OPTIONS := llw,check=no
BOARD_SYSTEM_FILESYSTEM_OPTIONS := nodev,nodiratime,relatime,errors=continue
BOARD_CACHE_DEVICE := /dev/block/stl8
BOARD_CACHE_FILESYSTEM := ext2
#BOARD_CACHE_FILESYSTEM_OPTIONS := llw,check=no,nosuid,nodev
BOARD_CACHE_FILESYSTEM_OPTIONS := nosuid,nodev,nodiratime,relatime,errors=continue
#BOARD_SDEXT_DEVICE := /dev/block/mmcblk0p2
BOARD_SDCARD_DEVICE_SECONDARY := NULL

BOARD_HAS_NO_MISC_PARTITION := true

BOARD_RECOVERY_DEVICE := /dev/block/bml9

BOARD_HAS_NO_RECOVERY_PARTITION := true

BOARD_LDPI_RECOVERY := true

#BOARD_HAS_NO_SELECT_BUTTON := true
