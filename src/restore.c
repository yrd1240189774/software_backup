#include "restore.h"
#include "main.h"
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
    // 保存当前工作目录
    char current_dir[256];
    GetCurrentDirectory(256, current_dir);

    // 获取备份文件的绝对路径，确保在切换目录后仍然可以访问
    char backup_file_abs[512];
    GetFullPathName(backup_file, sizeof(backup_file_abs), backup_file_abs, NULL);

    // 切换到目标路径，让解包函数直接将文件提取到目标路径
    if (!SetCurrentDirectory(target_path)) {
        return BACKUP_ERROR_PATH;
    }

    // 调用解包函数直接将文件提取到目标路径，使用绝对路径访问备份文件
    FileMetadata *files = NULL;
    int file_count = 0;
    
    // 先尝试使用MyPack算法解包
    BackupResult result = unpack_files(backup_file_abs, &files, &file_count, PACK_ALGORITHM_MYPACK);
    
    // 如果MyPack解包失败，尝试使用Tar算法
    if (result != BACKUP_SUCCESS) {
        result = unpack_files(backup_file_abs, &files, &file_count, PACK_ALGORITHM_TAR);
    }
    
    if (result != BACKUP_SUCCESS) {
        SetCurrentDirectory(current_dir);
        return result;
    }
    
    // 切换回原始工作目录
    SetCurrentDirectory(current_dir);
    
    free(files);
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

    // 解密文件（如果需要）
    char temp_file[512];
    const char *processed_file = options->backup_file;
    
    if (options->encrypt_enable) {
        snprintf(temp_file, sizeof(temp_file), "%s\\temp_decrypt", options->target_path);
        BackupResult result = decrypt_file(processed_file, temp_file, options->encrypt_algorithm, options->encrypt_key);
        if (result != BACKUP_SUCCESS) {
            return result;
        }
        processed_file = temp_file;
    }

    // 解压文件（如果需要）
    char temp_uncompress[512];
    const char *final_file = processed_file;
    
    // 这里需要根据备份文件的实际压缩情况判断是否需要解压
    // 目前简单处理，假设备份文件可能是压缩的
    snprintf(temp_uncompress, sizeof(temp_uncompress), "%s\\temp_uncompress.dat", options->target_path);
    BackupResult result = decompress_file(processed_file, temp_uncompress, COMPRESS_ALGORITHM_LZ77);
    if (result == BACKUP_SUCCESS) {
        final_file = temp_uncompress;
    } else {
        // 尝试用Huffman解压
        result = decompress_file(processed_file, temp_uncompress, COMPRESS_ALGORITHM_HAFF);
        if (result == BACKUP_SUCCESS) {
            final_file = temp_uncompress;
        } else {
            // 解压失败，可能不是压缩文件，直接使用原始文件
            final_file = processed_file;
        }
    }

    // 解包文件
    result = extract_files(final_file, options->target_path, options);
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
