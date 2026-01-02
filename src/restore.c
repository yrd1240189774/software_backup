#include "../include/restore.h"
#include "../include/main.h"
#include <windows.h>

// 辅助函数：创建目录
static int create_dir(const char *path) {
    char temp_path[512];
    strcpy(temp_path, path);
    
    // 替换所有的'/'为'\'
    for (int i = 0; temp_path[i] != '\0'; i++) {
        if (temp_path[i] == '/') {
            temp_path[i] = '\\';
        }
    }
    
    // 创建目录（递归）
    char *ptr = temp_path + 1;
    while ((ptr = strchr(ptr, '\\')) != NULL) {
        *ptr = '\0';
        CreateDirectory(temp_path, NULL);
        *ptr = '\\';
        ptr++;
    }
    
    return CreateDirectory(temp_path, NULL) != 0 || GetLastError() == ERROR_ALREADY_EXISTS;
}

// 恢复单个文件
BackupResult restore_single_file(const char *source, const char *target, const FileMetadata *metadata) {
    // 创建目标文件的目录
    char dir_path[512];
    strcpy(dir_path, target);
    char *last_slash = strrchr(dir_path, '\\');
    if (last_slash != NULL) {
        *last_slash = '\0';
        create_dir(dir_path);
    }
    
    // 复制文件
    if (CopyFile(source, target, FALSE) == 0) {
        return BACKUP_ERROR_FILE;
    }
    
    // 设置文件元数据
    return set_file_metadata(target, metadata);
}

// 解包并提取文件
BackupResult extract_files(const char *backup_file, const char *target_path, const RestoreOptions *options) {
    printf("DEBUG: Entering extract_files function\n");
    printf("DEBUG: Backup file: %s\n", backup_file);
    printf("DEBUG: Target path: %s\n", target_path);
    
    // 保存当前工作目录
    char current_dir[256];
    GetCurrentDirectory(256, current_dir);
    printf("DEBUG: Current directory: %s\n", current_dir);

    // 获取备份文件的绝对路径，确保在切换目录后仍然可以访问
    char backup_file_abs[512];
    GetFullPathName(backup_file, sizeof(backup_file_abs), backup_file_abs, NULL);
    printf("DEBUG: Backup file absolute path: %s\n", backup_file_abs);

    // 切换到目标路径，让解包函数直接将文件提取到目标路径
    printf("DEBUG: Switching to target directory: %s\n", target_path);
    if (!SetCurrentDirectory(target_path)) {
        printf("DEBUG: Failed to switch to target directory\n");
        return BACKUP_ERROR_PATH;
    }
    
    char new_current_dir[256];
    GetCurrentDirectory(256, new_current_dir);
    printf("DEBUG: Now in directory: %s\n", new_current_dir);

    // 调用解包函数直接将文件提取到目标路径，使用绝对路径访问备份文件
    FileMetadata *files = NULL;
    int file_count = 0;
    
    // 先尝试使用Tar算法解包
    printf("DEBUG: Trying to unpack with TAR algorithm\n");
    BackupResult result = unpack_files(backup_file_abs, &files, &file_count, PACK_ALGORITHM_TAR);
    printf("DEBUG: TAR unpack returned: %d, file count: %d\n", result, file_count);
    
    // 只有当解出文件时，才认为TAR解包成功，否则尝试ZIP算法
    if (result != BACKUP_SUCCESS || file_count == 0) {
        printf("DEBUG: TAR unpack failed or no files extracted, trying ZIP algorithm\n");
        result = unpack_files(backup_file_abs, &files, &file_count, PACK_ALGORITHM_ZIP);
        printf("DEBUG: ZIP unpack returned: %d, file count: %d\n", result, file_count);
    }
    

    
    if (result != BACKUP_SUCCESS) {
        printf("DEBUG: All unpack attempts failed\n");
        SetCurrentDirectory(current_dir);
        return result;
    }
    
    printf("DEBUG: Unpack successful, file count: %d\n", file_count);
    for (int i = 0; i < file_count; i++) {
        printf("DEBUG: Unpacked file %d: path=%s, name=%s, size=%lu\n", 
               i, files[i].path, files[i].name, files[i].size);
    }
    
    // 切换回原始工作目录
    printf("DEBUG: Switching back to original directory: %s\n", current_dir);
    SetCurrentDirectory(current_dir);
    
    free(files);
    printf("DEBUG: Exiting extract_files function with success\n");
    return BACKUP_SUCCESS;
}

// 还原主函数
BackupResult restore_data(const RestoreOptions *options) {
    if (options == NULL || options->backup_file[0] == 0 || options->target_path[0] == 0) {
        return BACKUP_ERROR_PARAM;
    }

    // 检查备份文件是否存在
    if (GetFileAttributes(options->backup_file) == INVALID_FILE_ATTRIBUTES) {
        return BACKUP_ERROR_PATH;
    }

    // 创建目标目录
    if (CreateDirectory(options->target_path, NULL) == 0 && GetLastError() != ERROR_ALREADY_EXISTS) {
        return BACKUP_ERROR_PATH;
    }

    BackupResult result;
    
    // 解密文件（如果需要）
    char temp_file[512];
    const char *processed_file = options->backup_file;
    
    if (options->encrypt_enable) {
        snprintf(temp_file, sizeof(temp_file), "%s\\temp_decrypt", options->target_path);
        result = decrypt_file(processed_file, temp_file, options->encrypt_key);
        if (result != BACKUP_SUCCESS) {
            return result;
        }
        processed_file = temp_file;
    }

    // 解压文件（如果需要）
    char temp_uncompress[512];
    const char *final_file = processed_file;
    
    // 执行解压步骤（不检查7z文件，因为已经不再支持7z）
    {
    snprintf(temp_uncompress, sizeof(temp_uncompress), "%s\\temp_uncompress.dat", options->target_path);
    
    // 首先尝试LZ77解压
    BackupResult lz77_result = decompress_file(processed_file, temp_uncompress, COMPRESS_ALGORITHM_LZ77);
    if (lz77_result == BACKUP_SUCCESS) {
        final_file = temp_uncompress;
    } else {
        // 尝试Huffman解压
        BackupResult haff_result = decompress_file(processed_file, temp_uncompress, COMPRESS_ALGORITHM_HAFF);
        if (haff_result == BACKUP_SUCCESS) {
            final_file = temp_uncompress;
        } else {
            // 解压失败，可能不是压缩文件，直接使用原始文件
            final_file = processed_file;
        }
    }
}

    // 解包文件
    printf("DEBUG: Calling extract_files function\n");
    printf("DEBUG: Final file: %s\n", final_file);
    printf("DEBUG: Target path: %s\n", options->target_path);
    
    result = extract_files(final_file, options->target_path, options);
    printf("DEBUG: extract_files returned: %d\n", result);
    
    if (result != BACKUP_SUCCESS) {
        // 清理临时文件
        if (options->encrypt_enable) {
            DeleteFile(temp_file);
        }
        if (final_file != processed_file) {
            DeleteFile(temp_uncompress);
        }
        return result;
    }

    // 清理临时文件
    if (options->encrypt_enable) {
        DeleteFile(temp_file);
    }
    if (final_file != processed_file) {
        DeleteFile(temp_uncompress);
    }

    return BACKUP_SUCCESS;
}
