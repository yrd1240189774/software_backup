#include "../include/filter.h"
#include <string.h>

// 主筛选函数
int filter_file(const FileMetadata *metadata, const BackupOptions *options) {
    if (metadata == NULL || options == NULL) {
        return 0;
    }

    // 按文件类型筛选
    if (!filter_by_type(metadata, options->file_types)) {
        return 0;
    }

    // 按时间筛选
    if (!filter_by_time(metadata, &options->create_time_range, &options->modify_time_range, &options->access_time_range)) {
        return 0;
    }

    // 按大小筛选
    if (!filter_by_size(metadata, &options->size_range)) {
        return 0;
    }

    // 按用户/组筛选
    if (!filter_by_user(metadata, &options->exclude_usergroup)) {
        return 0;
    }

    // 按文件名筛选
    if (!filter_by_filename(metadata, &options->exclude_filename)) {
        return 0;
    }

    // 按目录筛选
    if (!filter_by_directory(metadata, &options->exclude_directory)) {
        return 0;
    }

    return 1;
}

// 按文件类型筛选 - Windows文件系统版本
int filter_by_type(const FileMetadata *metadata, FileType file_types) {
    if (metadata == NULL) {
        return 0;
    }

    // 特殊处理FILE_TYPE_ALL
    if (file_types == FILE_TYPE_ALL) {
        return 1;
    }

    // 检查基础文件类型
    FileType base_type = metadata->type & 0x07; // 保留前3位：REGULAR(1), DIRECTORY(2), SYMLINK(4)
    
    // 检查是否匹配基础文件类型
    // 如果用户没有指定任何基础文件类型，则默认允许所有基础文件类型
    FileType requested_base_types = file_types & 0x07;
    if (requested_base_types != 0 && !(base_type & requested_base_types)) {
        return 0;
    }

    // 检查隐藏文件属性：如果用户没有指定FILE_TYPE_HIDDEN，那么隐藏属性不影响过滤
    // 如果用户指定了FILE_TYPE_HIDDEN，表示允许隐藏文件，而不是要求必须是隐藏文件
    // 即 FILE_TYPE_REGULAR | FILE_TYPE_HIDDEN 表示允许普通文件（包括隐藏的普通文件）
    if (!(file_types & FILE_TYPE_HIDDEN) && (metadata->type & FILE_TYPE_HIDDEN)) {
        // 用户没有允许隐藏文件，但当前文件是隐藏文件，过滤掉
        return 0;
    }

    // 检查系统文件属性：如果用户没有指定FILE_TYPE_SYSTEM，那么系统属性不影响过滤
    // 如果用户指定了FILE_TYPE_SYSTEM，表示允许系统文件，而不是要求必须是系统文件
    // 即 FILE_TYPE_REGULAR | FILE_TYPE_SYSTEM 表示允许普通文件（包括系统的普通文件）
    if (!(file_types & FILE_TYPE_SYSTEM) && (metadata->type & FILE_TYPE_SYSTEM)) {
        // 用户没有允许系统文件，但当前文件是系统文件，过滤掉
        return 0;
    }

    return 1;
}

// 按时间筛选
int filter_by_time(const FileMetadata *metadata, const TimeRange *create_range, const TimeRange *modify_range, const TimeRange *access_range) {
    if (metadata == NULL) {
        return 0;
    }

    // 检查创建时间
    if (create_range->enable) {
        if (metadata->create_time < create_range->start_time || metadata->create_time > create_range->end_time) {
            return 0;
        }
    }

    // 检查修改时间
    if (modify_range->enable) {
        if (metadata->modify_time < modify_range->start_time || metadata->modify_time > modify_range->end_time) {
            return 0;
        }
    }

    // 检查访问时间
    if (access_range->enable) {
        if (metadata->access_time < access_range->start_time || metadata->access_time > access_range->end_time) {
            return 0;
        }
    }

    return 1;
}

// 按大小筛选
int filter_by_size(const FileMetadata *metadata, const SizeRange *size_range) {
    if (metadata == NULL || size_range == NULL) {
        return 0;
    }

    // 如果max_size为0，不检查文件大小上限
    if (size_range->max_size == 0) {
        return (metadata->size >= size_range->min_size);
    }

    return (metadata->size >= size_range->min_size && metadata->size <= size_range->max_size);
}

// 按用户/组筛选
int filter_by_user(const FileMetadata *metadata, const ExcludeUserGroup *exclude_usergroup) {
    if (metadata == NULL) {
        return 0;
    }
    if (exclude_usergroup == NULL) {
        return 1; // 如果没有排除规则，允许所有文件
    }

    // 检查是否在排除用户列表中
    if (exclude_usergroup->enable_user) {
        for (int i = 0; i < exclude_usergroup->user_count; i++) {
            // 在Windows上，uid实际上是SIDs的简化，这里简化处理
            // 实际实现中需要将uid转换为用户名进行比较
            if (metadata->uid == atoi(exclude_usergroup->exclude_users[i])) {
                return 0;
            }
        }
    }

    // 检查是否在排除组列表中
    if (exclude_usergroup->enable_group) {
        for (int i = 0; i < exclude_usergroup->group_count; i++) {
            // 在Windows上，gid实际上是SIDs的简化，这里简化处理
            // 实际实现中需要将gid转换为组名进行比较
            if (metadata->gid == atoi(exclude_usergroup->exclude_groups[i])) {
                return 0;
            }
        }
    }

    return 1;
}

// 按文件名筛选
int filter_by_filename(const FileMetadata *metadata, const ExcludeFilename *exclude_filename) {
    if (metadata == NULL) {
        return 0;
    }
    if (exclude_filename == NULL) {
        return 1; // 如果没有排除规则，允许所有文件
    }

    // 检查排除文件名模式（简化实现，只支持精确匹配）
    if (exclude_filename->pattern[0] != 0) {
        if (strcmp(metadata->name, exclude_filename->pattern) == 0) {
            return 0; // 匹配到排除文件名，排除该文件
        }
    }

    return 1;
}

// 按目录筛选
int filter_by_directory(const FileMetadata *metadata, const ExcludeDirectory *exclude_directory) {
    if (metadata == NULL) {
        return 0;
    }

    // 检查是否在排除目录列表中
    if (exclude_directory != NULL) {
        for (int i = 0; i < exclude_directory->count; i++) {
            if (strstr(metadata->path, exclude_directory->paths[i]) != NULL) {
                return 0;
            }
        }
    }

    return 1;
}
