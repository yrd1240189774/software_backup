#include "pack.h"
#include "main.h"
#include <windows.h>

// MyPack打包实现
BackupResult mypack_pack(FILE *fp, const FileMetadata *files, int file_count) {
    PackHeader header;
    PackFileItem *items = NULL;
    unsigned long data_offset = 0;
    int i;

    // 分配文件项数组
    items = (PackFileItem *)malloc(file_count * sizeof(PackFileItem));
    if (items == NULL) {
        return BACKUP_ERROR_MEMORY;
    }

    // 计算头部大小和数据偏移量
    header.magic[0] = 'B';
    header.magic[1] = 'A';
    header.magic[2] = 'C';
    header.magic[3] = 'K';
    header.version = 1;
    header.file_count = file_count;
    header.header_size = sizeof(PackHeader) + file_count * sizeof(PackFileItem);
    data_offset = header.header_size;
    header.data_offset = data_offset;

    // 写入头部
    if (write_pack_header(fp, &header) != BACKUP_SUCCESS) {
        free(items);
        return BACKUP_ERROR_PACK;
    }

    // 写入文件项并计算偏移量
    for (i = 0; i < file_count; i++) {
        // 填充文件项
        strcpy(items[i].path, files[i].path);
        strcpy(items[i].name, files[i].name);
        items[i].type = files[i].type;
        items[i].size = files[i].size;
        items[i].offset = data_offset;
        items[i].create_time = files[i].create_time;
        items[i].modify_time = files[i].modify_time;
        items[i].access_time = files[i].access_time;
        items[i].mode = files[i].mode;
        items[i].uid = files[i].uid;
        items[i].gid = files[i].gid;
        strcpy(items[i].symlink_target, files[i].symlink_target);

        // 写入文件项
        if (write_pack_file_item(fp, &items[i]) != BACKUP_SUCCESS) {
            free(items);
            return BACKUP_ERROR_PACK;
        }

        // 更新数据偏移量
        data_offset += files[i].size;
    }

    // 写入文件数据
    for (i = 0; i < file_count; i++) {
        FILE *src_fp = fopen(files[i].path, "rb");
        if (src_fp == NULL) {
            free(items);
            return BACKUP_ERROR_FILE;
        }

        // 定位到文件数据位置
        if (fseek(fp, items[i].offset, SEEK_SET) != 0) {
            fclose(src_fp);
            free(items);
            return BACKUP_ERROR_FILE;
        }

        // 复制文件数据
        char buffer[4096];
        size_t bytes_read, bytes_written;
        while ((bytes_read = fread(buffer, 1, sizeof(buffer), src_fp)) > 0) {
            bytes_written = fwrite(buffer, 1, bytes_read, fp);
            if (bytes_written != bytes_read) {
                fclose(src_fp);
                free(items);
                return BACKUP_ERROR_FILE;
            }
        }

        fclose(src_fp);
    }

    // 清理资源
    free(items);
    return BACKUP_SUCCESS;
}

// Tar打包实现
BackupResult tar_pack(FILE *fp, const FileMetadata *files, int file_count) {
    // 简化的Tar格式实现
    char header[512];
    int i;

    for (i = 0; i < file_count; i++) {
        // 清空Tar文件头
        memset(header, 0, 512);
        
        // 文件名（100字节）
        strncpy(header, files[i].name, 100);
        
        // 文件模式（8字节，八进制）
        sprintf(header + 100, "%07o", files[i].mode);
        
        // UID（8字节，八进制）
        sprintf(header + 108, "%07o", files[i].uid);
        
        // GID（8字节，八进制）
        sprintf(header + 116, "%07o", files[i].gid);
        
        // 文件大小（12字节，八进制）
        sprintf(header + 124, "%011lo", (unsigned long)files[i].size);
        
        // 修改时间（12字节，八进制）
        sprintf(header + 136, "%011lo", (unsigned long)files[i].modify_time);
        
        // 类型标志（1字节）
        header[156] = '0'; // 普通文件
        
        // 魔术字（8字节）
        strcpy(header + 257, "ustar  ");
        
        // 写入文件头
        if (fwrite(header, 1, 512, fp) != 512) {
            return BACKUP_ERROR_FILE;
        }
        
        // 写入文件数据
        FILE *src_fp = fopen(files[i].path, "rb");
        if (src_fp == NULL) {
            return BACKUP_ERROR_FILE;
        }
        
        char buffer[4096];
        size_t bytes_read, bytes_written;
        size_t remaining = files[i].size;
        while (remaining > 0) {
            bytes_read = fread(buffer, 1, (remaining < sizeof(buffer)) ? remaining : sizeof(buffer), src_fp);
            if (bytes_read == 0) {
                fclose(src_fp);
                return BACKUP_ERROR_FILE;
            }
            
            bytes_written = fwrite(buffer, 1, bytes_read, fp);
            if (bytes_written != bytes_read) {
                fclose(src_fp);
                return BACKUP_ERROR_FILE;
            }
            
            remaining -= bytes_written;
        }
        
        fclose(src_fp);
        
        // 填充到512字节的倍数
        if (files[i].size % 512 != 0) {
            size_t padding = 512 - (files[i].size % 512);
            memset(buffer, 0, padding);
            if (fwrite(buffer, 1, padding, fp) != padding) {
                return BACKUP_ERROR_FILE;
            }
        }
    }
    
    // 写入两个512字节的结束块
    char end_block[512] = {0};
    if (fwrite(end_block, 1, 512, fp) != 512 || 
        fwrite(end_block, 1, 512, fp) != 512) {
        return BACKUP_ERROR_FILE;
    }
    
    return BACKUP_SUCCESS;
}

// 打包文件
BackupResult pack_files(const char *output_path, const FileMetadata *files, int file_count, PackAlgorithm algorithm) {
    FILE *fp = NULL;
    BackupResult result = BACKUP_SUCCESS;

    // 检查参数
    if (output_path == NULL || files == NULL || file_count <= 0) {
        return BACKUP_ERROR_PARAM;
    }

    // 打开输出文件
    fp = fopen(output_path, "wb");
    if (fp == NULL) {
        return BACKUP_ERROR_FILE;
    }

    // 根据算法选择打包方式
    switch (algorithm) {
        case PACK_ALGORITHM_MYPACK:
            result = mypack_pack(fp, files, file_count);
            break;
        case PACK_ALGORITHM_TAR:
            result = tar_pack(fp, files, file_count);
            break;
        default:
            result = mypack_pack(fp, files, file_count);
            break;
    }

    // 清理资源
    fclose(fp);
    return result;
}

// MyPack解包实现
BackupResult mypack_unpack(FILE *fp, FileMetadata **files, int *file_count) {
    PackHeader header;
    PackFileItem *items = NULL;
    int i;

    // 读取头部
    if (read_pack_header(fp, &header) != BACKUP_SUCCESS) {
        return BACKUP_ERROR_PACK;
    }

    // 检查魔术字
    if (memcmp(header.magic, "BACK", 4) != 0) {
        return BACKUP_ERROR_PACK;
    }

    // 分配文件项数组
    items = (PackFileItem *)malloc(header.file_count * sizeof(PackFileItem));
    if (items == NULL) {
        return BACKUP_ERROR_MEMORY;
    }

    // 读取文件项
    for (i = 0; i < header.file_count; i++) {
        if (read_pack_file_item(fp, &items[i]) != BACKUP_SUCCESS) {
            free(items);
            return BACKUP_ERROR_PACK;
        }
    }

    // 分配输出文件元数据数组
    *files = (FileMetadata *)malloc(header.file_count * sizeof(FileMetadata));
    if (*files == NULL) {
        free(items);
        return BACKUP_ERROR_MEMORY;
    }
    *file_count = header.file_count;

    // 提取文件数据
    for (i = 0; i < header.file_count; i++) {
        // 填充输出文件元数据
        strcpy((*files)[i].path, items[i].path);
        strcpy((*files)[i].name, items[i].name);
        (*files)[i].type = items[i].type;
        (*files)[i].size = items[i].size;
        (*files)[i].create_time = items[i].create_time;
        (*files)[i].modify_time = items[i].modify_time;
        (*files)[i].access_time = items[i].access_time;
        (*files)[i].mode = items[i].mode;
        (*files)[i].uid = items[i].uid;
        (*files)[i].gid = items[i].gid;
        strcpy((*files)[i].symlink_target, items[i].symlink_target);

        // 定位到文件数据位置
        if (fseek(fp, items[i].offset, SEEK_SET) != 0) {
            free(items);
            free(*files);
            return BACKUP_ERROR_FILE;
        }

        // 创建目录结构
        char dir_path[256];
        strcpy(dir_path, items[i].path);
        char *last_slash = strrchr(dir_path, '\\');
        if (last_slash != NULL) {
            *last_slash = '\0';
            // 递归创建目录
            char temp_path[256];
            strcpy(temp_path, dir_path);
            
            // 创建目录（递归）
            char *ptr = temp_path + 1;
            while ((ptr = strchr(ptr, '\\')) != NULL) {
                *ptr = '\0';
                CreateDirectory(temp_path, NULL);
                *ptr = '\\';
                ptr++;
            }
            
            // 创建最终目录
            CreateDirectory(temp_path, NULL);
        }

        // 写入文件数据
        FILE *dst_fp = fopen(items[i].path, "wb");
        if (dst_fp == NULL) {
            free(items);
            free(*files);
            return BACKUP_ERROR_FILE;
        }

        char buffer[4096];
        size_t bytes_read, bytes_written;
        size_t remaining = items[i].size;
        while (remaining > 0) {
            bytes_read = fread(buffer, 1, (remaining < sizeof(buffer)) ? remaining : sizeof(buffer), fp);
            if (bytes_read == 0) {
                fclose(dst_fp);
                free(items);
                free(*files);
                return BACKUP_ERROR_FILE;
            }

            bytes_written = fwrite(buffer, 1, bytes_read, dst_fp);
            if (bytes_written != bytes_read) {
                fclose(dst_fp);
                free(items);
                free(*files);
                return BACKUP_ERROR_FILE;
            }

            remaining -= bytes_written;
        }

        fclose(dst_fp);
    }

    // 清理资源
    free(items);
    return BACKUP_SUCCESS;
}

// Tar解包实现
BackupResult tar_unpack(FILE *fp, FileMetadata **files, int *file_count) {
    // 简化的Tar解包实现
    char header[512];
    size_t bytes_read;
    int count = 0;
    int max_files = 100;
    
    // 分配初始文件元数据数组
    *files = (FileMetadata *)malloc(max_files * sizeof(FileMetadata));
    if (*files == NULL) {
        return BACKUP_ERROR_MEMORY;
    }
    
    while (1) {
        // 读取Tar文件头
        bytes_read = fread(header, 1, 512, fp);
        if (bytes_read != 512) {
            // 检查是否遇到文件结束
            if (feof(fp)) {
                break;
            } else {
                free(*files);
                return BACKUP_ERROR_FILE;
            }
        }
        
        // 检查文件头是否全零（结束块）
        int all_zero = 1;
        for (int i = 0; i < 512; i++) {
            if (header[i] != 0) {
                all_zero = 0;
                break;
            }
        }
        if (all_zero) {
            // 再读一个块，确认是否是结束标记（两个连续的全零块）
            bytes_read = fread(header, 1, 512, fp);
            if (bytes_read == 512) {
                // 检查第二个块是否也是全零
                int second_all_zero = 1;
                for (int i = 0; i < 512; i++) {
                    if (header[i] != 0) {
                        second_all_zero = 0;
                        break;
                    }
                }
                if (second_all_zero) {
                    // 找到结束标记，退出循环
                    break;
                } else {
                    // 不是结束标记，将文件指针移回
                    fseek(fp, -512, SEEK_CUR);
                }
            } else {
                // 读取失败，退出循环
                break;
            }
        }
        
        // 解析文件名
        char filename[256] = {0};
        strncpy(filename, header, 100);
        
        // 跳过空文件名（可能是填充块）
        if (filename[0] == '\0') {
            continue;
        }
        
        // 解析文件大小
        char size_str[13] = {0};
        strncpy(size_str, header + 124, 12);
        unsigned long size = 0;
        sscanf(size_str, "%lo", &size);
        
        // 解析文件模式
        char mode_str[9] = {0};
        strncpy(mode_str, header + 100, 8);
        int mode = 0;
        sscanf(mode_str, "%o", &mode); // 使用%o而不是%lo，因为mode是int类型
        
        // 解析UID和GID
        char uid_str[9] = {0};
        char gid_str[9] = {0};
        strncpy(uid_str, header + 108, 8);
        strncpy(gid_str, header + 116, 8);
        int uid = 0, gid = 0;
        sscanf(uid_str, "%o", &uid); // 使用%o而不是%lo
        sscanf(gid_str, "%o", &gid); // 使用%o而不是%lo
        
        // 解析修改时间
        char mtime_str[13] = {0};
        strncpy(mtime_str, header + 136, 12);
        unsigned long mtime = 0;
        sscanf(mtime_str, "%lo", &mtime);
        
        // 扩展文件元数据数组（如果需要）
        if (count >= max_files) {
            max_files *= 2;
            FileMetadata *new_files = (FileMetadata *)realloc(*files, max_files * sizeof(FileMetadata));
            if (new_files == NULL) {
                free(*files);
                return BACKUP_ERROR_MEMORY;
            }
            *files = new_files;
        }
        
        // 填充文件元数据
        strcpy((*files)[count].path, filename);
        strcpy((*files)[count].name, filename);
        (*files)[count].type = FILE_TYPE_REGULAR;
        (*files)[count].size = size;
        (*files)[count].create_time = mtime;
        (*files)[count].modify_time = mtime;
        (*files)[count].access_time = mtime;
        (*files)[count].mode = mode;
        (*files)[count].uid = uid;
        (*files)[count].gid = gid;
        (*files)[count].symlink_target[0] = '\0';
        
        // 写入文件数据
        FILE *dst_fp = fopen(filename, "wb");
        if (dst_fp == NULL) {
            free(*files);
            return BACKUP_ERROR_FILE;
        }
        
        char buffer[4096];
        size_t bytes_written;
        size_t remaining = size;
        while (remaining > 0) {
            bytes_read = fread(buffer, 1, (remaining < sizeof(buffer)) ? remaining : sizeof(buffer), fp);
            if (bytes_read == 0) {
                fclose(dst_fp);
                free(*files);
                return BACKUP_ERROR_FILE;
            }
            
            bytes_written = fwrite(buffer, 1, bytes_read, dst_fp);
            if (bytes_written != bytes_read) {
                fclose(dst_fp);
                free(*files);
                return BACKUP_ERROR_FILE;
            }
            
            remaining -= bytes_written;
        }
        
        fclose(dst_fp);
        
        // 跳过文件数据后的填充
        if (size % 512 != 0) {
            size_t padding = 512 - (size % 512);
            if (fseek(fp, padding, SEEK_CUR) != 0) {
                free(*files);
                return BACKUP_ERROR_FILE;
            }
        }
        
        count++;
    }
    
    // 调整数组大小
    *files = (FileMetadata *)realloc(*files, count * sizeof(FileMetadata));
    if (*files == NULL && count > 0) {
        return BACKUP_ERROR_MEMORY;
    }
    
    *file_count = count;
    return BACKUP_SUCCESS;
}

// 解包文件
BackupResult unpack_files(const char *input_path, FileMetadata **files, int *file_count, PackAlgorithm algorithm) {
    FILE *fp = NULL;
    BackupResult result = BACKUP_SUCCESS;

    // 检查参数
    if (input_path == NULL || files == NULL || file_count == NULL) {
        return BACKUP_ERROR_PARAM;
    }

    // 打开输入文件
    fp = fopen(input_path, "rb");
    if (fp == NULL) {
        return BACKUP_ERROR_FILE;
    }

    // 根据算法选择解包方式
    switch (algorithm) {
        case PACK_ALGORITHM_MYPACK:
            result = mypack_unpack(fp, files, file_count);
            break;
        case PACK_ALGORITHM_TAR:
            result = tar_unpack(fp, files, file_count);
            break;
        default:
            result = mypack_unpack(fp, files, file_count);
            break;
    }

    // 清理资源
    fclose(fp);
    return result;
}

// 写入打包文件头部
BackupResult write_pack_header(FILE *fp, const PackHeader *header) {
    if (fp == NULL || header == NULL) {
        return BACKUP_ERROR_PARAM;
    }

    size_t bytes_written = fwrite(header, sizeof(PackHeader), 1, fp);
    if (bytes_written != 1) {
        return BACKUP_ERROR_FILE;
    }

    return BACKUP_SUCCESS;
}

// 读取打包文件头部
BackupResult read_pack_header(FILE *fp, PackHeader *header) {
    if (fp == NULL || header == NULL) {
        return BACKUP_ERROR_PARAM;
    }

    size_t bytes_read = fread(header, sizeof(PackHeader), 1, fp);
    if (bytes_read != 1) {
        return BACKUP_ERROR_FILE;
    }

    return BACKUP_SUCCESS;
}

// 写入文件项
BackupResult write_pack_file_item(FILE *fp, const PackFileItem *item) {
    if (fp == NULL || item == NULL) {
        return BACKUP_ERROR_PARAM;
    }

    size_t bytes_written = fwrite(item, sizeof(PackFileItem), 1, fp);
    if (bytes_written != 1) {
        return BACKUP_ERROR_FILE;
    }

    return BACKUP_SUCCESS;
}

// 读取文件项
BackupResult read_pack_file_item(FILE *fp, PackFileItem *item) {
    if (fp == NULL || item == NULL) {
        return BACKUP_ERROR_PARAM;
    }

    size_t bytes_read = fread(item, sizeof(PackFileItem), 1, fp);
    if (bytes_read != 1) {
        return BACKUP_ERROR_FILE;
    }

    return BACKUP_SUCCESS;
}