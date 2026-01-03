#ifndef MAIN_H
#define MAIN_H

#include "types.h"

// 全局函数声明

// 备份模块
BackupResult backup_data(const BackupOptions *options);

// 还原模块
BackupResult restore_data(const RestoreOptions *options);

// 文件筛选模块
int filter_file(const FileMetadata *metadata, const BackupOptions *options);

// 打包解包模块
BackupResult pack_files(const char *output_path, const FileMetadata *files, int file_count, PackAlgorithm algorithm, CompressAlgorithm compress_algorithm);
BackupResult unpack_files(const char *input_path, FileMetadata **files, int *file_count, PackAlgorithm algorithm);

// 压缩解压模块
BackupResult compress_file(const char *input_path, const char *output_path, CompressAlgorithm algorithm);
BackupResult decompress_file(const char *input_path, const char *output_path, CompressAlgorithm algorithm);

// 加密解密模块
BackupResult encrypt_file(const char *input_path, const char *output_path, const char *password);
BackupResult decrypt_file(const char *input_path, const char *output_path, const char *password);

// 元数据处理模块
BackupResult get_file_metadata(const char *path, FileMetadata *metadata);
BackupResult set_file_metadata(const char *path, const FileMetadata *metadata);

// 定时备份模块
BackupResult schedule_backup(const ScheduledBackupOptions *options);
void run_scheduled_backup(const ScheduledBackupOptions *options);
int check_schedule_due(const ScheduleConfig *config);
void update_next_run_time(ScheduleConfig *config);

// 数据清理模块
BackupResult cleanup_old_backups(const CleanupConfig *config);
int is_backup_outdated(const char *backup_path, int keep_days);
time_t get_file_modification_time(const char *path);

// 配置管理模块
BackupResult save_schedule_config(const ScheduledBackupOptions *options, const char *config_path);
BackupResult load_schedule_config(ScheduledBackupOptions *options, const char *config_path);


#endif // MAIN_H
