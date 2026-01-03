#include "../include/types.h"
#include "../include/filter.h"
#include "../include/backup.h"
#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>

// 辅助函数：检查路径是否是符号链接
int is_symlink(const char *path)
{
    DWORD attr = GetFileAttributes(path);
    return (attr != INVALID_FILE_ATTRIBUTES &&
            (attr & FILE_ATTRIBUTE_REPARSE_POINT) &&
            (attr & FILE_ATTRIBUTE_DIRECTORY) == 0);
}

// 目录遍历算法
BackupResult traverse_directory(const char *root_path, FileMetadata **files, int *file_count, const BackupOptions *options)
{
    if (root_path == NULL || files == NULL || file_count == NULL)
    {
        return BACKUP_ERROR_PARAM;
    }

    // 初始化文件数组
    int capacity = 100;
    *files = (FileMetadata *)malloc(capacity * sizeof(FileMetadata));
    if (*files == NULL)
    {
        return BACKUP_ERROR_MEMORY;
    }
    *file_count = 0;

    // 遍历栈，用于非递归遍历目录
    typedef struct
    {
        char full_path[256];
        char rel_path[256];
        HANDLE find_handle;
        WIN32_FIND_DATA find_data;
        int first_entry;
    } DirStackItem;
    DirStackItem dir_stack[100];
    int stack_top = -1;

    // 初始化遍历栈
    WIN32_FIND_DATA find_data;
    char search_path[256];
    sprintf(search_path, "%s\\*", root_path);
    printf("DEBUG: Searching path: %s\n", search_path);  // 添加调试信息
    HANDLE find_handle = FindFirstFile(search_path, &find_data);
    if (find_handle == INVALID_HANDLE_VALUE)
    {
        printf("DEBUG: FindFirstFile failed, error: %d\n", GetLastError());  // 添加调试信息
        free(*files);
        return BACKUP_ERROR_PATH;
    }

    stack_top++;
    strcpy(dir_stack[stack_top].full_path, root_path);
    strcpy(dir_stack[stack_top].rel_path, ".");
    dir_stack[stack_top].find_handle = find_handle;
    memcpy(&dir_stack[stack_top].find_data, &find_data, sizeof(WIN32_FIND_DATA));
    dir_stack[stack_top].first_entry = 1;

    // 非递归遍历目录
    while (stack_top >= 0)
    {
        DirStackItem *current = &dir_stack[stack_top];

        if (current->first_entry)
        {
            current->first_entry = 0;
        }
        else
        {
            if (!FindNextFile(current->find_handle, &current->find_data))
            {
                FindClose(current->find_handle);
                stack_top--;
                continue;
            }
        }

        // 跳过当前目录和父目录
        if (strcmp(current->find_data.cFileName, ".") == 0 || strcmp(current->find_data.cFileName, "..") == 0)
        {
            continue;
        }

        // 构建完整的绝对路径
        char full_item_path[256];
        sprintf(full_item_path, "%s\\%s", current->full_path, current->find_data.cFileName);
        printf("DEBUG: Processing item: %s\n", full_item_path);  // 添加调试信息

        // 构建相对路径
        char rel_item_path[256];
        if (strcmp(current->rel_path, ".") == 0)
        {
            strcpy(rel_item_path, current->find_data.cFileName);
        }
        else
        {
            sprintf(rel_item_path, "%s\\%s", current->rel_path, current->find_data.cFileName);
        }

        // 获取文件属性
        DWORD attr = GetFileAttributes(full_item_path);
        if (attr == INVALID_FILE_ATTRIBUTES)
        {
            printf("DEBUG: GetFileAttributes failed for: %s\n", full_item_path);  // 添加调试信息
            continue;
        }

        // 获取文件元数据
        FileMetadata metadata;
        memset(&metadata, 0, sizeof(FileMetadata));

        // 设置文件类型
        if (attr & FILE_ATTRIBUTE_DIRECTORY)
        {
            metadata.type = FILE_TYPE_DIRECTORY;
        }
        else if (attr & FILE_ATTRIBUTE_REPARSE_POINT)
        {
            metadata.type = FILE_TYPE_SYMLINK;
        }
        else
        {
            metadata.type = FILE_TYPE_REGULAR;
        }

        // 设置文件名和路径
        // 使用相对路径作为name字段，这样在打包时可以保留目录结构
        strncpy(metadata.name, rel_item_path, sizeof(metadata.name) - 1);
        metadata.name[sizeof(metadata.name) - 1] = '\0';

        // 同时确保path字段也设置为相对路径
        strncpy(metadata.path, rel_item_path, sizeof(metadata.path) - 1);
        metadata.path[sizeof(metadata.path) - 1] = '\0';

        // 获取文件大小
        if (metadata.type == FILE_TYPE_REGULAR)
        {
            // 使用CreateFile和GetFileSizeEx获取文件大小，避免打开文件时的锁问题
            HANDLE file_handle = CreateFile(full_item_path, GENERIC_READ, 
                                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                           NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (file_handle != INVALID_HANDLE_VALUE)
            {
                LARGE_INTEGER file_size;
                if (GetFileSizeEx(file_handle, &file_size))
                {
                    metadata.size = (unsigned long)file_size.QuadPart;
                }
                CloseHandle(file_handle);
            }
            else
            {
                printf("DEBUG: CreateFile failed for: %s, error: %d\n", full_item_path, GetLastError());  // 添加调试信息
            }
        }
        else
        {
            metadata.size = 0;  // 目录的大小设为0
        }

        // 获取文件时间
        HANDLE time_handle = CreateFile(full_item_path, GENERIC_READ, 
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (time_handle != INVALID_HANDLE_VALUE)
        {
            FILETIME create_time, access_time, write_time;
            if (GetFileTime(time_handle, &create_time, &access_time, &write_time))
            {
                // 将FILETIME转换为time_t
                SYSTEMTIME sys_time;
                struct tm tm_time;
                
                // 创建时间
                FileTimeToSystemTime(&create_time, &sys_time);
                tm_time.tm_sec = sys_time.wSecond;
                tm_time.tm_min = sys_time.wMinute;
                tm_time.tm_hour = sys_time.wHour;
                tm_time.tm_mday = sys_time.wDay;
                tm_time.tm_mon = sys_time.wMonth - 1;
                tm_time.tm_year = sys_time.wYear - 1900;
                tm_time.tm_wday = sys_time.wDayOfWeek;
                metadata.create_time = mktime(&tm_time);
                
                // 修改时间
                FileTimeToSystemTime(&write_time, &sys_time);
                tm_time.tm_sec = sys_time.wSecond;
                tm_time.tm_min = sys_time.wMinute;
                tm_time.tm_hour = sys_time.wHour;
                tm_time.tm_mday = sys_time.wDay;
                tm_time.tm_mon = sys_time.wMonth - 1;
                tm_time.tm_year = sys_time.wYear - 1900;
                tm_time.tm_wday = sys_time.wDayOfWeek;
                metadata.modify_time = mktime(&tm_time);
                
                // 访问时间
                FileTimeToSystemTime(&access_time, &sys_time);
                tm_time.tm_sec = sys_time.wSecond;
                tm_time.tm_min = sys_time.wMinute;
                tm_time.tm_hour = sys_time.wHour;
                tm_time.tm_mday = sys_time.wDay;
                tm_time.tm_mon = sys_time.wMonth - 1;
                tm_time.tm_year = sys_time.wYear - 1900;
                tm_time.tm_wday = sys_time.wDayOfWeek;
                metadata.access_time = mktime(&tm_time);
            }
            CloseHandle(time_handle);
        }

        // 设置默认权限
        metadata.mode = 0644;
        metadata.uid = 0;
        metadata.gid = 0;

        // 处理符号链接
        if (metadata.type == FILE_TYPE_SYMLINK)
        {
            // 简化处理：只记录符号链接类型，不处理目标路径
            metadata.symlink_target[0] = '\0';
        }

        // 处理目录：如果是目录，加入遍历栈，不添加到结果列表
        if (attr & FILE_ATTRIBUTE_DIRECTORY && 
            !(attr & FILE_ATTRIBUTE_REPARSE_POINT))
        {
            // 这是一个目录，将其加入遍历栈
            char subdir_full_path[256];
            char subdir_rel_path[256];
            sprintf(subdir_full_path, "%s\\%s", current->full_path, current->find_data.cFileName);
            sprintf(subdir_rel_path, "%s\\%s", current->rel_path, current->find_data.cFileName);
            
            // 打开子目录
            char subdir_search[256];
            sprintf(subdir_search, "%s\\*", subdir_full_path);
            HANDLE subdir_handle = FindFirstFile(subdir_search, &find_data);
            if (subdir_handle != INVALID_HANDLE_VALUE)
            {
                stack_top++;
                strcpy(dir_stack[stack_top].full_path, subdir_full_path);
                strcpy(dir_stack[stack_top].rel_path, subdir_rel_path);
                dir_stack[stack_top].find_handle = subdir_handle;
                memcpy(&dir_stack[stack_top].find_data, &find_data, sizeof(WIN32_FIND_DATA));
                dir_stack[stack_top].first_entry = 1;
            }
            // 跳过当前目录，继续处理下一个项
            continue;
        }
        
        // 只处理非目录文件
        
        // 数据过滤
        if (options != NULL && !filter_file(&metadata, options))
        {
            printf("DEBUG: File filtered out: %s\n", metadata.name);  // 添加调试信息
            continue;
        }

        // 添加到文件列表
        if (*file_count >= capacity)
        {
            capacity *= 2;
            FileMetadata *new_files = (FileMetadata *)realloc(*files, capacity * sizeof(FileMetadata));
            if (new_files == NULL)
            {
                free(*files);
                return BACKUP_ERROR_MEMORY;
            }
            *files = new_files;
        }
        (*files)[*file_count] = metadata;
        (*file_count)++;
        printf("DEBUG: Added file to list: %s, size: %lu\n", metadata.name, metadata.size);  // 添加调试信息
    }

    return BACKUP_SUCCESS;
}