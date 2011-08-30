extern int signature_check_enabled;
extern int script_assert_enabled;

void
toggle_signature_check();

void
toggle_script_asserts();

void
show_choose_zip_menu();

int
do_nandroid_backup(const char* backup_name);

int
do_nandroid_restore();

void
show_nandroid_restore_menu();

static int
erase_root1(const char *root);

static void
wipe_data1(int confirm);

void
show_nandroid_menu();

void
updatemenu();

void
show_wipe_menu();

void
backup_rom();

void
show_partition_menu();

void
show_choose_zip_menu();

int
install_zip(const char* packagefilepath);

int
__system(const char *command);

void
show_advanced_menu();

void
show_multi_boot_menu();

int
format_unknown_device(const char* root);

void
wipe_battery_stats();

void create_fstab();
