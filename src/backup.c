#include "../include/backup.h"
#include "../include/main.h"
#include "../include/traverse.h"
#include <windows.h>
#include <tchar.h>

// 辅助函数：递归创建目录
int create_directory_recursive(const char *path) {
    char temp_path[512];
    strcpy(temp_path, path);
    
    // 替换所有的'/'为'\'
    for (int i = 0; temp_path[i] != '\0'; i++) {
        if (temp_path[i] == '/') {
            temp_path[i] = '\\';
        }
    }
    
    // 递归创建目录
    char *ptr = temp_path + 1;
    while ((ptr = strchr(ptr, '\\')) != NULL) {
        *ptr = '\0';
        CreateDirectory(temp_path, NULL);
        *ptr = '\\';
        ptr++;
    }
    
    // 创建最终目录
    return CreateDirectory(temp_path, NULL) != 0 || GetLastError() == ERROR_ALREADY_EXISTS;
}

// 备份主函数
BackupResult backup_data(const BackupOptions *options) {
    printf("DEBUG: Entering backup_data function\n");
    printf("DEBUG: Source path: %s\n", options->source_path);
    printf("DEBUG: Target path: %s\n", options->target_path);
    
    if (options == NULL || options->source_path[0] == 0 || options->target_path[0] == 0) {
        printf("DEBUG: Invalid parameters\n");
        return BACKUP_ERROR_PARAM;
    }

    // 检查源路径是否存在
    DWORD file_attr = GetFileAttributes(options->source_path);
    if (file_attr == INVALID_FILE_ATTRIBUTES) {
        printf("DEBUG: Source path does not exist\n");
        return BACKUP_ERROR_PATH;
    }
    printf("DEBUG: Source path exists, attributes: %d\n", file_attr);

    // 创建目标目录
    if (CreateDirectory(options->target_path, NULL) == 0 && GetLastError() != ERROR_ALREADY_EXISTS) {
        printf("DEBUG: Failed to create target directory, error: %d\n", GetLastError());
        return BACKUP_ERROR_PATH;
    }
    printf("DEBUG: Target directory created successfully\n");

    // 遍历目录，收集文件
    FileMetadata *files = NULL;
    int file_count = 0;
    BackupResult result;
    
    if (file_attr & FILE_ATTRIBUTE_DIRECTORY) {
        // 源路径是目录，遍历目录
        printf("DEBUG: Source is directory, traversing...\n");
        result = traverse_directory(options->source_path, &files, &file_count, options);
        if (result != BACKUP_SUCCESS) {
            printf("DEBUG: traverse_directory failed with error: %d\n", result);
            return result;
        }
    } else {
        // 源路径是文件，直接处理单个文件
        printf("DEBUG: Source is file, processing directly...\n");
        files = (FileMetadata *)malloc(sizeof(FileMetadata));
        if (files == NULL) {
            printf("DEBUG: Memory allocation failed\n");
            return BACKUP_ERROR_MEMORY;
        }
        
        result = get_file_metadata(options->source_path, &files[0]);
        if (result != BACKUP_SUCCESS) {
            printf("DEBUG: get_file_metadata failed with error: %d\n", result);
            free(files);
            return result;
        }
        
        // 筛选文件
        if (filter_file(&files[0], options)) {
            file_count = 1;
        } else {
            free(files);
            files = NULL;
            file_count = 0;
        }
    }
    printf("DEBUG: Directory traversal completed, found %d files\n", file_count);
    
    // 检查文件列表
    for (int i = 0; i < file_count; i++) {
        printf("DEBUG: Found file %d: path=%s, name=%s, size=%lu\n", 
               i, files[i].path, files[i].name, files[i].size);
    }

    // 检查是否有文件需要打包
    if (file_count == 0) {
        printf("DEBUG: No files to pack\n");
        free(files);
        return BACKUP_ERROR_NO_FILES;
    }

    // 构建打包文件路径，确保使用绝对路径
    char pack_file_path[512];
    char compress_file_path[512];
    char encrypt_file_path[512];
    char current_pack_path[512];
    
    // 生成绝对路径的打包文件名
    char target_abs[512];
    GetFullPathName(options->target_path, sizeof(target_abs), target_abs, NULL);
    sprintf(pack_file_path, "%s\\backup.dat", target_abs);
    printf("DEBUG: Pack file path: %s\n", pack_file_path);
    
    // 保存当前工作目录
    char current_dir[256];
    GetCurrentDirectory(256, current_dir);
    
    // 切换到源目录，以便pack_files能正确找到相对路径的文件
    if (!SetCurrentDirectory(options->source_path)) {
        free(files);
        return BACKUP_ERROR_PATH;
    }
    
    // 打包文件
    result = pack_files(pack_file_path, files, file_count, options->pack_algorithm, options->compress_algorithm);
    
    // 切换回原始工作目录
    SetCurrentDirectory(current_dir);
    
    if (result != BACKUP_SUCCESS) {
        free(files);
        return result;
    }

    // 保存当前打包文件路径，用于后续处理
    strcpy(current_pack_path, pack_file_path);
    
    // 压缩打包文件（如果需要）
    if (options->compress_algorithm != COMPRESS_ALGORITHM_NONE) {
        sprintf(compress_file_path, "%s\\backup_compressed.dat", options->target_path);
        
        result = compress_file(current_pack_path, compress_file_path, options->compress_algorithm);
        if (result != BACKUP_SUCCESS) {
            free(files);
            return result;
        }
        
        // 更新当前处理的文件路径为压缩后的文件
        strcpy(current_pack_path, compress_file_path);
    }

    // 加密文件（如果需要）
    if (options->encrypt_enable) {
        sprintf(encrypt_file_path, "%s\\backup_encrypted.dat", options->target_path);
        
        result = encrypt_file(current_pack_path, encrypt_file_path, options->encrypt_key);
        if (result != BACKUP_SUCCESS) {
            free(files);
            return result;
        }
        
        // 更新当前处理的文件路径为加密后的文件
        strcpy(current_pack_path, encrypt_file_path);
        
        // 将加密后的文件重命名为backup.dat，以便restore能正确找到
        DeleteFile(pack_file_path);
        if (MoveFile(encrypt_file_path, pack_file_path) == 0) {
            printf("DEBUG: Failed to rename encrypted file to backup.dat, error: %d\n", GetLastError());
            free(files);
            return BACKUP_ERROR_FILE;
        }
    } else if (strcmp(current_pack_path, pack_file_path) != 0) {
        // 如果没有加密，但有压缩，将压缩后的文件重命名为backup.dat
        DeleteFile(pack_file_path);
        if (MoveFile(current_pack_path, pack_file_path) == 0) {
            printf("DEBUG: Failed to rename compressed file to backup.dat, error: %d\n", GetLastError());
            free(files);
            return BACKUP_ERROR_FILE;
        }
    }

    free(files);
    return BACKUP_SUCCESS;
}

// 复制文件
BackupResult copy_file(const char *source, const char *target) {
    if (CopyFile(source, target, FALSE) == 0) {
        return BACKUP_ERROR_FILE;
    }
    return BACKUP_SUCCESS;
}
