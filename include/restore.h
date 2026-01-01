#ifndef RESTORE_H
#define RESTORE_H

#include "types.h"

// 还原模块内部函数声明
BackupResult extract_files(const char *backup_file, const char *target_path, const RestoreOptions *options);
BackupResult restore_single_file(const char *source, const char *target, const FileMetadata *metadata);

#endif // RESTORE_H
