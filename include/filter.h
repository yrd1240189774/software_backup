#ifndef FILTER_H
#define FILTER_H

#include "types.h"

// 主筛选函数声明
int filter_file(const FileMetadata *metadata, const BackupOptions *options);

// 文件筛选模块内部函数声明
int filter_by_type(const FileMetadata *metadata, FileType file_types);
int filter_by_time(const FileMetadata *metadata, const TimeRange *create_range, const TimeRange *modify_range, const TimeRange *access_range);
int filter_by_size(const FileMetadata *metadata, const SizeRange *size_range);
int filter_by_user(const FileMetadata *metadata, const ExcludeUserGroup *exclude_usergroup);
int filter_by_filename(const FileMetadata *metadata, const ExcludeFilename *exclude_filename);
int filter_by_directory(const FileMetadata *metadata, const ExcludeDirectory *exclude_directory);

#endif // FILTER_H
