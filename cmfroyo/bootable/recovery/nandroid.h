#ifndef NANDROID_H
#define NANDROID_H

int nandroid_main(int argc, char** argv);
int nandroid_backup(const char* backup_path);
int nandroid_backup_boot(const char* backup_path);
int nandroid_backup_system(const char* backup_path);
int nandroid_backup_data(const char* backup_path);
int nandroid_backup_cache(const char* backup_path);
int nandroid_backup_sd(const char* backup_path);
int nandroid_backup_androidSecure(const char* backup_path);
int nandroid_restore(const char* backup_path, int restore_boot, int restore_system, int restore_data, int restore_cache, int restore_sdext);
int nandroid_restore_system(const char* backup_path, int restore_system);
int nandroid_restore_data(const char* backup_path, int restore_data);
int nandroid_restore_sd(const char* backup_path, int restore_sdext);
int nandroid_restore_androidSecure(const char* backup_path, int restore_androidSecure);
void nandroid_generate_timestamp_path(char* backup_path);

#endif
