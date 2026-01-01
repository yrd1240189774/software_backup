#ifndef TRAVERSE_H
#define TRAVERSE_H

#include "types.h"

// 目录遍历算法函数声明
BackupResult traverse_directory(const char *root_path, FileMetadata **files, int *file_count, const BackupOptions *options);

// 目录遍历过滤函数声明
int filter_file(const FileMetadata *metadata, const BackupOptions *options);

#endif // TRAVERSE_H