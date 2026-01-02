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

#endif // MAIN_H
