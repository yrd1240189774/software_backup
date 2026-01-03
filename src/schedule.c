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
    
    BackupResult result = backup_data(options);
    if (result == BACKUP_SUCCESS) {
        printf("备份成功完成！\n");
    } else {
        printf("备份失败，错误码: %d\n", result);
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

// 定时备份主函数
BackupResult schedule_backup(const ScheduledBackupOptions *options) {
    ScheduledBackupOptions local_options = *options;
    
    // 设置下次运行时间
    update_next_run_time(&(local_options.schedule_config));
    
    printf("定时备份已启动，下次运行时间: ");
    struct tm *tm_info = localtime(&(local_options.schedule_config.next_run_time));
    printf("%04d-%02d-%02d %02d:%02d:%02d\n", 
           tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
           tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
    
    int backup_count = 0;
    const int max_backups = 3; // 限制测试时执行的备份次数
    
    // 持续运行定时备份（在测试模式下限制次数）
    while (local_options.schedule_config.enable && backup_count < max_backups) {
        if (check_schedule_due(&(local_options.schedule_config))) {
            run_scheduled_backup(&local_options);
            
            // 更新上次运行时间和下次运行时间
            local_options.schedule_config.last_run_time = time(NULL);
            update_next_run_time(&(local_options.schedule_config));
            
            printf("下次运行时间: ");
            struct tm *tm_info = localtime(&(local_options.schedule_config.next_run_time));
            printf("%04d-%02d-%02d %02d:%02d:%02d\n", 
                   tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
                   tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
            
            backup_count++;
        }
        
        // 等待1分钟再检查
        Sleep(60000);  // Windows下Sleep函数参数单位是毫秒
    }
    
    printf("定时备份测试已完成 %d 次备份\n", backup_count);
    return BACKUP_SUCCESS;
}