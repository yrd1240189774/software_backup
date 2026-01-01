#include "main.h"
#include <windows.h>

// 获取文件元数据
BackupResult get_file_metadata(const char *path, FileMetadata *metadata) {
    if (path == NULL || metadata == NULL) {
        return BACKUP_ERROR_PARAM;
    }

    WIN32_FILE_ATTRIBUTE_DATA file_attr;
    FILETIME ft_create, ft_access, ft_write;
    SYSTEMTIME st_create, st_access, st_write;

    // 获取文件属性
    if (GetFileAttributesEx(path, GetFileExInfoStandard, &file_attr) == 0) {
        return BACKUP_ERROR_FILE;
    }

    // 初始化元数据
    memset(metadata, 0, sizeof(FileMetadata));
    strcpy(metadata->path, path);

    // 提取文件名
    const char *filename = strrchr(path, '\\');
    if (filename != NULL) {
        strcpy(metadata->name, filename + 1);
    } else {
        strcpy(metadata->name, path);
    }

    // 确定文件类型
    if (file_attr.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        metadata->type = FILE_TYPE_DIRECTORY;
    } else if (file_attr.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
        // 符号链接（Windows上的实现）
        metadata->type = FILE_TYPE_SYMLINK;
        // 获取符号链接目标（简化实现）
        strcpy(metadata->symlink_target, "symlink_target");
    } else {
        metadata->type = FILE_TYPE_REGULAR;
    }

    // 获取文件大小
    metadata->size = ((unsigned long long)file_attr.nFileSizeHigh << 32) | file_attr.nFileSizeLow;

    // 转换文件时间
    FileTimeToSystemTime(&file_attr.ftCreationTime, &st_create);
    FileTimeToSystemTime(&file_attr.ftLastAccessTime, &st_access);
    FileTimeToSystemTime(&file_attr.ftLastWriteTime, &st_write);

    // 转换为time_t格式
    struct tm tm_create = {
        st_create.wSecond, st_create.wMinute, st_create.wHour,
        st_create.wDay, st_create.wMonth - 1, st_create.wYear - 1900
    };
    metadata->create_time = mktime(&tm_create);

    struct tm tm_access = {
        st_access.wSecond, st_access.wMinute, st_access.wHour,
        st_access.wDay, st_access.wMonth - 1, st_access.wYear - 1900
    };
    metadata->access_time = mktime(&tm_access);

    struct tm tm_write = {
        st_write.wSecond, st_write.wMinute, st_write.wHour,
        st_write.wDay, st_write.wMonth - 1, st_write.wYear - 1900
    };
    metadata->modify_time = mktime(&tm_write);

    // 文件权限（Windows上简化处理）
    metadata->mode = 0644; // 默认权限

    // 用户ID和组ID（Windows上简化处理）
    metadata->uid = 0;
    metadata->gid = 0;

    return BACKUP_SUCCESS;
}

// 设置文件元数据
BackupResult set_file_metadata(const char *path, const FileMetadata *metadata) {
    if (path == NULL || metadata == NULL) {
        return BACKUP_ERROR_PARAM;
    }

    // 设置文件时间
    SYSTEMTIME st_create, st_access, st_write;
    FILETIME ft_create, ft_access, ft_write;

    // 转换create_time为SYSTEMTIME
    struct tm *tm_create = localtime(&metadata->create_time);
    st_create.wYear = tm_create->tm_year + 1900;
    st_create.wMonth = tm_create->tm_mon + 1;
    st_create.wDay = tm_create->tm_mday;
    st_create.wHour = tm_create->tm_hour;
    st_create.wMinute = tm_create->tm_min;
    st_create.wSecond = tm_create->tm_sec;
    st_create.wMilliseconds = 0;

    // 转换access_time为SYSTEMTIME
    struct tm *tm_access = localtime(&metadata->access_time);
    st_access.wYear = tm_access->tm_year + 1900;
    st_access.wMonth = tm_access->tm_mon + 1;
    st_access.wDay = tm_access->tm_mday;
    st_access.wHour = tm_access->tm_hour;
    st_access.wMinute = tm_access->tm_min;
    st_access.wSecond = tm_access->tm_sec;
    st_access.wMilliseconds = 0;

    // 转换modify_time为SYSTEMTIME
    struct tm *tm_write = localtime(&metadata->modify_time);
    st_write.wYear = tm_write->tm_year + 1900;
    st_write.wMonth = tm_write->tm_mon + 1;
    st_write.wDay = tm_write->tm_mday;
    st_write.wHour = tm_write->tm_hour;
    st_write.wMinute = tm_write->tm_min;
    st_write.wSecond = tm_write->tm_sec;
    st_write.wMilliseconds = 0;

    // 转换为FILETIME
    SystemTimeToFileTime(&st_create, &ft_create);
    SystemTimeToFileTime(&st_access, &ft_access);
    SystemTimeToFileTime(&st_write, &ft_write);

    // 设置文件时间
    if (SetFileTime((HANDLE)CreateFile(path, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL),
                    &ft_create, &ft_access, &ft_write) == 0) {
        return BACKUP_ERROR_FILE;
    }

    // 在Windows上，文件权限、用户ID和组ID的设置比较复杂，这里简化处理
    // 实际实现中需要使用SetFileSecurity等函数

    return BACKUP_SUCCESS;
}
