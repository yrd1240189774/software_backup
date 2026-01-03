#include "../include/main.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include <io.h>
#include <sys/stat.h>

// 获取文件修改时间
time_t get_file_modification_time(const char *path) {
    struct stat file_stat;
    if (stat(path, &file_stat) == 0) {
        return file_stat.st_mtime;
    }
    return 0;
}

// 检查备份是否过期
int is_backup_outdated(const char *backup_path, int keep_days) {
    if (keep_days <= 0) {
        return 0;  // 如果没有设置保留天数，则不认为过期
    }
    
    time_t now = time(NULL);
    time_t file_time = get_file_modification_time(backup_path);
    time_t diff_seconds = now - file_time;
    int diff_days = diff_seconds / (60 * 60 * 24);
    
    return (diff_days > keep_days);
}

// 比较函数，用于排序文件
int compare_files_by_time(const void *a, const void *b) {
    const char *file_a = *(const char **)a;
    const char *file_b = *(const char **)b;
    
    time_t time_a = get_file_modification_time(file_a);
    time_t time_b = get_file_modification_time(file_b);
    
    if (time_a < time_b) return 1;  // 最新的文件排在前面
    if (time_a > time_b) return -1;
    return 0;
}

// 获取目录中的备份文件列表
char **get_backup_files(const char *directory, int *count) {
    char search_path[512];
    sprintf(search_path, "%s\\*", directory);
    
    WIN32_FIND_DATA find_data;
    HANDLE h_find = FindFirstFile(search_path, &find_data);
    
    if (h_find == INVALID_HANDLE_VALUE) {
        *count = 0;
        return NULL;
    }
    
    char **files = malloc(1000 * sizeof(char *));  // 假设最多1000个文件
    int file_count = 0;
    
    do {
        if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            // 这是一个文件，不是目录
            files[file_count] = malloc(512);
            sprintf(files[file_count], "%s\\%s", directory, find_data.cFileName);
            file_count++;
        }
    } while (FindNextFile(h_find, &find_data) && file_count < 1000);
    
    FindClose(h_find);
    
    *count = file_count;
    return files;
}

// 数据清理主函数
BackupResult cleanup_old_backups(const CleanupConfig *config) {
    if (!config->enable) {
        printf("数据清理未启用\n");
        return BACKUP_SUCCESS;
    }
    
    printf("开始清理备份目录: %s\n", config->backup_directory);
    
    int file_count = 0;
    char **files = get_backup_files(config->backup_directory, &file_count);
    
    if (file_count <= 0) {
        printf("目录中没有找到文件\n");
        return BACKUP_SUCCESS;
    }
    
    printf("找到 %d 个文件\n", file_count);
    
    // 按修改时间排序（最新的在前）
    qsort(files, file_count, sizeof(char *), compare_files_by_time);
    
    int deleted_count = 0;
    
    // 清理过期文件（按时间）
    if (config->keep_days > 0) {
        for (int i = 0; i < file_count; i++) {
            if (is_backup_outdated(files[i], config->keep_days)) {
                printf("删除过期文件: %s\n", files[i]);
                if (DeleteFile(files[i])) {
                    deleted_count++;
                } else {
                    printf("删除失败: %s, 错误码: %d\n", files[i], GetLastError());
                }
            }
        }
    }
    
    // 如果设置了最大备份数，删除多余的备份
    if (config->max_backup_count > 0 && file_count > config->max_backup_count) {
        for (int i = config->max_backup_count; i < file_count; i++) {
            printf("删除超出数量限制的文件: %s\n", files[i]);
            if (DeleteFile(files[i])) {
                deleted_count++;
            } else {
                printf("删除失败: %s, 错误码: %d\n", files[i], GetLastError());
            }
        }
    }
    
    // 释放内存
    for (int i = 0; i < file_count; i++) {
        if (files[i]) {
            free(files[i]);
        }
    }
    if (files) {
        free(files);
    }
    
    printf("数据清理完成，共删除 %d 个文件\n", deleted_count);
    return BACKUP_SUCCESS;
}