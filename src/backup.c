#include "backup.h"
#include "main.h"
#include "traverse.h"
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
    if (options == NULL || options->source_path[0] == 0 || options->target_path[0] == 0) {
        return BACKUP_ERROR_PARAM;
    }

    // 检查源路径是否存在
    DWORD file_attr = GetFileAttributes(options->source_path);
    if (file_attr == INVALID_FILE_ATTRIBUTES) {
        return BACKUP_ERROR_PATH;
    }

    // 创建目标目录
    if (CreateDirectory(options->target_path, NULL) == 0 && GetLastError() != ERROR_ALREADY_EXISTS) {
        return BACKUP_ERROR_PATH;
    }

    // 遍历目录，收集文件
    FileMetadata *files = NULL;
    int file_count = 0;
    BackupResult result;
    
    if (file_attr & FILE_ATTRIBUTE_DIRECTORY) {
        // 源路径是目录，遍历目录
        result = traverse_directory(options->source_path, &files, &file_count, options);
        if (result != BACKUP_SUCCESS) {
            return result;
        }
    } else {
        // 源路径是文件，直接处理单个文件
        files = (FileMetadata *)malloc(sizeof(FileMetadata));
        if (files == NULL) {
            return BACKUP_ERROR_MEMORY;
        }
        
        result = get_file_metadata(options->source_path, &files[0]);
        if (result != BACKUP_SUCCESS) {
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

    // 检查是否有文件需要打包
    if (file_count == 0) {
        free(files);
        return BACKUP_ERROR_NO_FILES;
    }

    // 构建打包文件路径，确保使用绝对路径
    char pack_file_path[512];
    char compress_file_path[512];
    char encrypt_file_path[512];
    char current_pack_path[512];
    
    // 生成绝对路径的打包文件名
    GetFullPathName(options->target_path, sizeof(pack_file_path), pack_file_path, NULL);
    strcat(pack_file_path, "\\backup.dat");
    
    // 保存当前工作目录
    char current_dir[256];
    GetCurrentDirectory(256, current_dir);
    
    // 切换到源目录，以便pack_files能正确找到相对路径的文件
    if (!SetCurrentDirectory(options->source_path)) {
        free(files);
        return BACKUP_ERROR_PATH;
    }
    
    // 打包文件
    result = pack_files(pack_file_path, files, file_count, options->pack_algorithm);
    
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
        
        result = encrypt_file(current_pack_path, encrypt_file_path, options->encrypt_algorithm, options->encrypt_key);
        if (result != BACKUP_SUCCESS) {
            free(files);
            return result;
        }
        
        // 更新当前处理的文件路径为加密后的文件
        strcpy(current_pack_path, encrypt_file_path);
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
