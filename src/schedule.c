#include "../include/main.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>

// 更新下次运行时间
void update_next_run_time(ScheduleConfig *config) {
    if (config->interval_minutes > 0) {
        time_t now = time(NULL);
        config->next_run_time = now + (config->interval_minutes * 60);
    }
}

// 检查是否到达定时备份时间
int check_schedule_due(const ScheduleConfig *config) {
    time_t now = time(NULL);
    return (now >= config->next_run_time);
}

// 运行一次备份
void run_single_backup(const BackupOptions *options) {
    printf("执行备份任务...\n");
    printf("源路径: %s\n", options->source_path);
    printf("目标路径: %s\n", options->target_path);
    
    // 添加压缩和加密选项的调试输出
    printf("打包算法: %s\n", options->pack_algorithm == PACK_ALGORITHM_TAR ? "TAR" : "ZIP");
    const char* compress_name[] = {"NONE", "HAFF", "LZ77"};
    printf("压缩算法: %s\n", compress_name[options->compress_algorithm]);
    printf("加密: %s\n", options->encrypt_enable ? "启用" : "禁用");
    if (options->encrypt_enable) {
        const char* encrypt_name[] = {"NONE", "AES", "DES"};
        printf("加密算法: %s\n", encrypt_name[options->encrypt_algorithm]);
    }
    
    // 确保源路径存在，避免BACKUP_ERROR_FILE错误
    DWORD file_attr = GetFileAttributes(options->source_path);
    if (file_attr == INVALID_FILE_ATTRIBUTES) {
        printf("源路径不存在: %s\n", options->source_path);
        return;
    }
    
    // 确保目标路径存在
    if (CreateDirectory(options->target_path, NULL) == 0 && GetLastError() != ERROR_ALREADY_EXISTS) {
        printf("创建目标路径失败: %s\n", options->target_path);
        return;
    }
    
    BackupResult result = backup_data(options);
    if (result == BACKUP_SUCCESS) {
        printf("备份成功完成！\n");
    } else {
        printf("备份失败，错误码: %d\n", result);
        if (result == BACKUP_ERROR_FILE) {
            printf("文件操作失败，请检查路径权限和存在性\n");
        }
    }
}

// 运行定时备份
void run_scheduled_backup(const ScheduledBackupOptions *options) {
    printf("开始执行定时备份任务...\n");
    
    // 执行备份
    run_single_backup(&(options->backup_options));
    
    // 如果启用了清理，执行数据清理
    if (options->cleanup_config.enable) {
        printf("执行数据清理...\n");
        cleanup_old_backups(&(options->cleanup_config));
    }
}

// 定时备份线程函数
DWORD WINAPI schedule_thread_func(LPVOID lpParam) {
    printf("[DEBUG] 进入schedule_thread_func线程函数\n");
    
    ScheduledBackupOptions *options = (ScheduledBackupOptions *)lpParam;
    ScheduledBackupOptions local_options = *options;
    
    // 设置下次运行时间
    printf("[DEBUG] 设置初始下次运行时间\n");
    update_next_run_time(&(local_options.schedule_config));
    
    printf("[INFO] 定时备份已启动，下次运行时间: ");
    struct tm *tm_info = localtime(&(local_options.schedule_config.next_run_time));
    printf("%04d-%02d-%02d %02d:%02d:%02d\n", 
           tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
           tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
    
    // 持续运行定时备份，直到被取消
    int loop_count = 0;
    while (g_schedule_is_running) {
        loop_count++;
        printf("[DEBUG] 循环执行第 %d 次，运行状态: %d\n", loop_count, g_schedule_is_running);
        
        time_t now = time(NULL);
        printf("[DEBUG] 当前时间: %s", ctime(&now));
        printf("[DEBUG] 下次运行时间: %s", ctime(&(local_options.schedule_config.next_run_time)));
        
        if (check_schedule_due(&(local_options.schedule_config))) {
            printf("[INFO] 到达备份时间，开始执行备份任务\n");
            run_scheduled_backup(&local_options);
            
            // 更新上次运行时间和下次运行时间
            local_options.schedule_config.last_run_time = now;
            printf("[DEBUG] 更新下次运行时间\n");
            update_next_run_time(&(local_options.schedule_config));
            
            printf("[INFO] 备份完成，下次运行时间: ");
            struct tm *tm_info = localtime(&(local_options.schedule_config.next_run_time));
            printf("%04d-%02d-%02d %02d:%02d:%02d\n", 
                   tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
                   tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
        } else {
            printf("[DEBUG] 尚未到达备份时间，继续等待\n");
        }
        
        // 等待10秒再检查，提高响应速度
        printf("[DEBUG] 等待10秒后再次检查...\n");
        Sleep(10000);  // Windows下Sleep函数参数单位是毫秒
    }
    
    printf("[INFO] 定时备份线程已停止\n");
    return 0;
}

// 定时备份主函数 - 非阻塞实现
BackupResult schedule_backup(const ScheduledBackupOptions *options) {
    printf("[DEBUG] 进入schedule_backup函数\n");
    
    // 检查是否已经在运行
    if (g_schedule_is_running) {
        printf("[ERROR] 定时备份已经在运行中\n");
        return BACKUP_ERROR_PARAM;
    }
    
    // 保存当前配置
    memcpy(&g_current_schedule_options, options, sizeof(ScheduledBackupOptions));
    
    // 输出详细配置信息
    printf("[DEBUG] 定时备份配置：\n");
    printf("[DEBUG]   源路径: %s\n", options->backup_options.source_path);
    printf("[DEBUG]   目标路径: %s\n", options->backup_options.target_path);
    printf("[DEBUG]   打包算法: %s\n", options->backup_options.pack_algorithm == PACK_ALGORITHM_TAR ? "TAR" : "ZIP");
    printf("[DEBUG]   压缩算法: %s\n", options->backup_options.compress_algorithm == COMPRESS_ALGORITHM_NONE ? "NONE" : 
           options->backup_options.compress_algorithm == COMPRESS_ALGORITHM_HAFF ? "HAFF" : "LZ77");
    printf("[DEBUG]   加密: %s\n", options->backup_options.encrypt_enable ? "启用" : "禁用");
    printf("[DEBUG]   间隔小时: %d\n", options->schedule_config.interval_hours);
    printf("[DEBUG]   间隔分钟: %d\n", options->schedule_config.interval_minutes);
    printf("[DEBUG]   保留备份数: %d\n", options->cleanup_config.max_backup_count);
    
    // 启动定时备份线程
    g_schedule_thread = CreateThread(
        NULL,                   // 默认安全属性
        0,                      // 默认栈大小
        schedule_thread_func,    // 线程函数
        &g_current_schedule_options, // 线程参数
        0,                      // 立即运行
        NULL                    // 不返回线程ID
    );
    
    if (g_schedule_thread == NULL) {
        printf("[ERROR] 创建定时备份线程失败，错误码: %lu\n", GetLastError());
        return BACKUP_ERROR_MEMORY;
    }
    
    // 设置运行状态
    g_schedule_is_running = 1;
    
    printf("[INFO] 定时备份线程已启动，线程ID: %p\n", g_schedule_thread);
    return BACKUP_SUCCESS;
}

// 取消正在运行的定时备份
BackupResult cancel_schedule_backup(void) {
    printf("[DEBUG] 进入cancel_schedule_backup函数\n");
    
    if (!g_schedule_is_running || g_schedule_thread == NULL) {
        printf("[INFO] 没有正在运行的定时备份\n");
        return BACKUP_ERROR_PARAM;
    }
    
    // 设置停止标志
    printf("[INFO] 设置定时备份停止标志\n");
    g_schedule_is_running = 0;
    
    // 等待线程结束
    printf("[INFO] 等待定时备份线程结束...\n");
    DWORD wait_result = WaitForSingleObject(g_schedule_thread, 5000); // 等待5秒
    if (wait_result == WAIT_TIMEOUT) {
        // 超时，强制终止线程
        printf("[WARNING] 等待线程超时，将强制终止\n");
        TerminateThread(g_schedule_thread, 0);
        printf("[INFO] 定时备份线程已强制终止\n");
    } else if (wait_result == WAIT_OBJECT_0) {
        printf("[INFO] 定时备份线程已正常结束\n");
    } else {
        printf("[ERROR] 等待线程时发生错误，错误码: %lu\n", GetLastError());
        TerminateThread(g_schedule_thread, 0);
        printf("[INFO] 定时备份线程已强制终止\n");
    }
    
    // 关闭线程句柄
    printf("[DEBUG] 关闭线程句柄\n");
    CloseHandle(g_schedule_thread);
    g_schedule_thread = NULL;
    
    // 清空当前配置
    printf("[DEBUG] 清空当前配置\n");
    memset(&g_current_schedule_options, 0, sizeof(ScheduledBackupOptions));
    
    printf("[INFO] 定时备份已成功取消\n");
    return BACKUP_SUCCESS;
}