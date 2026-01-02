#include "../include/pack.h"
#include "../include/main.h"
#include "../include/compress.h"
#include <windows.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

// ZIP文件格式常量
#define ZIP_LOCAL_FILE_HEADER_SIGNATURE 0x04034B50
#define ZIP_CENTRAL_DIR_HEADER_SIGNATURE 0x02014B50
#define ZIP_END_CENTRAL_DIR_SIGNATURE 0x06054B50
#define ZIP_COMPRESSION_METHOD_NONE 0

// ZIP本地文件头结构
#pragma pack(push, 1)
typedef struct
{
    uint32_t signature;          // 本地文件头签名 (0x04034B50)
    uint16_t version_needed;     // 解压所需版本
    uint16_t flags;              // 通用位标记
    uint16_t compression_method; // 压缩方法 (0表示不压缩)
    uint16_t last_mod_time;      // 最后修改时间
    uint16_t last_mod_date;      // 最后修改日期
    uint32_t crc32;              // CRC-32校验值
    uint32_t compressed_size;    // 压缩后大小
    uint32_t uncompressed_size;  // 未压缩大小
    uint16_t filename_length;    // 文件名长度
    uint16_t extra_field_length; // 额外字段长度
} ZipLocalFileHeader;

// ZIP中央目录头结构
typedef struct
{
    uint32_t signature;           // 中央目录头签名 (0x02014B50)
    uint16_t created_version;     // 创建者版本
    uint16_t version_needed;      // 解压所需版本
    uint16_t flags;               // 通用位标记
    uint16_t compression_method;  // 压缩方法
    uint16_t last_mod_time;       // 最后修改时间
    uint16_t last_mod_date;       // 最后修改日期
    uint32_t crc32;               // CRC-32校验值
    uint32_t compressed_size;     // 压缩后大小
    uint32_t uncompressed_size;   // 未压缩大小
    uint16_t filename_length;     // 文件名长度
    uint16_t extra_field_length;  // 额外字段长度
    uint16_t file_comment_length; // 文件注释长度
    uint16_t disk_number;         // 磁盘号
    uint16_t internal_attrs;      // 内部文件属性
    uint32_t external_attrs;      // 外部文件属性
    uint32_t local_header_offset; // 本地文件头偏移
} ZipCentralDirHeader;

// ZIP中央目录结束记录结构
typedef struct
{
    uint32_t signature;          // 中央目录结束签名 (0x06054B50)
    uint16_t disk_number;        // 当前磁盘号
    uint16_t start_disk_number;  // 中央目录开始的磁盘号
    uint16_t entries_on_disk;    // 当前磁盘上的条目数
    uint16_t total_entries;      // 中央目录中的总条目数
    uint32_t central_dir_size;   // 中央目录大小
    uint32_t central_dir_offset; // 中央目录偏移
    uint16_t comment_length;     // 注释长度
} ZipEndCentralDir;
#pragma pack(pop)

// CRC32计算函数 (简化实现)
static uint32_t calculate_crc32(const void *data, size_t length)
{
    uint32_t crc = 0xFFFFFFFF;
    const uint8_t *bytes = (const uint8_t *)data;

    for (size_t i = 0; i < length; i++)
    {
        crc ^= bytes[i];
        for (int j = 0; j < 8; j++)
        {
            if (crc & 1)
            {
                crc = (crc >> 1) ^ 0xEDB88320;
            }
            else
            {
                crc >>= 1;
            }
        }
    }

    return crc ^ 0xFFFFFFFF;
}

// 将时间转换为ZIP格式的日期和时间
static void convert_time_to_zip_format(time_t t, uint16_t *date, uint16_t *time)
{
    struct tm *tm = localtime(&t);

    // 时间格式: 小时(5位) + 分钟(6位) + 秒/2(5位)
    *time = ((tm->tm_hour & 0x1F) << 11) |
            ((tm->tm_min & 0x3F) << 5) |
            ((tm->tm_sec / 2) & 0x1F);

    // 日期格式: 年-1980(7位) + 月(4位) + 日(5位)
    *date = (((tm->tm_year + 1900) - 1980) & 0x7F) << 9 |
            ((tm->tm_mon + 1) & 0x0F) << 5 |
            (tm->tm_mday & 0x1F);
}

// Tar打包实现
BackupResult tar_pack(FILE *fp, const FileMetadata *files, int file_count)
{
    // 简化的Tar格式实现
    char header[512];
    int i;

    for (i = 0; i < file_count; i++)
    {
        // 清空Tar文件头
        memset(header, 0, 512);


        // 简化为：
        char relative_name[101] = {0};
        // 直接使用name字段，它现在包含相对路径
        strncpy(relative_name, files[i].name, sizeof(relative_name) - 1);
        relative_name[100] = '\0'; // 确保字符串结束

        // 文件名（100字节）
        strncpy(header, relative_name, 100);

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
        if (fwrite(header, 1, 512, fp) != 512)
        {
            return BACKUP_ERROR_FILE;
        }

        // 写入文件数据
        FILE *src_fp = fopen(files[i].path, "rb");
        if (src_fp == NULL)
        {
            return BACKUP_ERROR_FILE;
        }

        char buffer[4096];
        size_t bytes_read, bytes_written;
        size_t remaining = files[i].size;
        while (remaining > 0)
        {
            bytes_read = fread(buffer, 1, (remaining < sizeof(buffer)) ? remaining : sizeof(buffer), src_fp);
            if (bytes_read == 0)
            {
                fclose(src_fp);
                return BACKUP_ERROR_FILE;
            }

            bytes_written = fwrite(buffer, 1, bytes_read, fp);
            if (bytes_written != bytes_read)
            {
                fclose(src_fp);
                return BACKUP_ERROR_FILE;
            }

            remaining -= bytes_written;
        }

        fclose(src_fp);

        // 填充到512字节的倍数
        if (files[i].size % 512 != 0)
        {
            size_t padding = 512 - (files[i].size % 512);
            memset(buffer, 0, padding);
            if (fwrite(buffer, 1, padding, fp) != padding)
            {
                return BACKUP_ERROR_FILE;
            }
        }
    }

    // 写入两个512字节的结束块
    char end_block[512] = {0};
    if (fwrite(end_block, 1, 512, fp) != 512 ||
        fwrite(end_block, 1, 512, fp) != 512)
    {
        return BACKUP_ERROR_FILE;
    }

    return BACKUP_SUCCESS;
}

// 辅助函数：递归创建目录
static int create_directory_recursive(const char *path)
{
    char temp_path[256];
    strcpy(temp_path, path);

    // 替换所有的'/'为'\'
    for (int i = 0; temp_path[i] != '\0'; i++)
    {
        if (temp_path[i] == '/')
        {
            temp_path[i] = '\\';
        }
    }

    // 递归创建目录
    char *ptr = temp_path + 1;
    while ((ptr = strchr(ptr, '\\')) != NULL)
    {
        *ptr = '\0';
        CreateDirectory(temp_path, NULL);
        *ptr = '\\';
        ptr++;
    }

    // 创建最终目录
    return CreateDirectory(temp_path, NULL) != 0 || GetLastError() == ERROR_ALREADY_EXISTS;
}

// ZIP解包实现
BackupResult zip_unpack(FILE *fp, FileMetadata **files, int *file_count)
{
    int max_files = 100;
    int count = 0;

    // 分配初始文件元数据数组
    *files = (FileMetadata *)malloc(max_files * sizeof(FileMetadata));
    if (*files == NULL)
    {
        return BACKUP_ERROR_MEMORY;
    }

    // 重置文件指针到开头
    fseek(fp, 0, SEEK_SET);

    // 查找所有本地文件头
    while (1)
    {
        // 读取签名
        uint32_t signature;
        if (fread(&signature, sizeof(signature), 1, fp) != 1)
        {
            break; // 文件结束
        }

        // 检查是否是本地文件头
        if (signature != ZIP_LOCAL_FILE_HEADER_SIGNATURE)
        {
            // 不是本地文件头，可能是中央目录或其他结构，结束遍历
            break;
        }

        // 读取本地文件头剩余部分
        ZipLocalFileHeader local_header;
        local_header.signature = signature;

        if (fread(&local_header.version_needed, sizeof(local_header) - sizeof(local_header.signature), 1, fp) != 1)
        {
            free(*files);
            return BACKUP_ERROR_FILE;
        }

        // 读取文件名
        char filename[256] = {0};
        if (fread(filename, 1, local_header.filename_length, fp) != local_header.filename_length)
        {
            free(*files);
            return BACKUP_ERROR_FILE;
        }

        // 跳过额外字段
        if (fseek(fp, local_header.extra_field_length, SEEK_CUR) != 0)
        {
            free(*files);
            return BACKUP_ERROR_FILE;
        }

        // 检查是否需要扩展文件数组
        if (count >= max_files)
        {
            max_files *= 2;
            FileMetadata *new_files = (FileMetadata *)realloc(*files, max_files * sizeof(FileMetadata));
            if (new_files == NULL)
            {
                free(*files);
                return BACKUP_ERROR_MEMORY;
            }
            *files = new_files;
        }

        // 保存文件数据
        FILE *dst_fp = fopen(filename, "wb");
        if (dst_fp == NULL)
        {
            free(*files);
            return BACKUP_ERROR_FILE;
        }

        // 读取并写入文件数据
        char buffer[4096];
        uint32_t remaining = local_header.uncompressed_size;
        while (remaining > 0)
        {
            size_t bytes_to_read = (remaining < sizeof(buffer)) ? remaining : sizeof(buffer);
            size_t bytes_read = fread(buffer, 1, bytes_to_read, fp);
            if (bytes_read == 0)
            {
                fclose(dst_fp);
                free(*files);
                return BACKUP_ERROR_FILE;
            }

            size_t bytes_written = fwrite(buffer, 1, bytes_read, dst_fp);
            if (bytes_written != bytes_read)
            {
                fclose(dst_fp);
                free(*files);
                return BACKUP_ERROR_FILE;
            }

            remaining -= bytes_read;
        }

        fclose(dst_fp);

        // 填充文件元数据
        strcpy((*files)[count].path, filename);
        strcpy((*files)[count].name, filename);
        (*files)[count].type = FILE_TYPE_REGULAR;
        (*files)[count].size = local_header.uncompressed_size;
        (*files)[count].create_time = time(NULL);
        (*files)[count].modify_time = time(NULL);
        (*files)[count].access_time = time(NULL);
        (*files)[count].mode = 0644;
        (*files)[count].uid = 0;
        (*files)[count].gid = 0;
        (*files)[count].symlink_target[0] = '\0';

        count++;
    }

    // 调整文件数组大小
    *files = (FileMetadata *)realloc(*files, count * sizeof(FileMetadata));
    if (*files == NULL && count > 0)
    {
        return BACKUP_ERROR_MEMORY;
    }

    *file_count = count;
    return BACKUP_SUCCESS;
}

// ZIP打包实现（不压缩）
BackupResult zip_pack(FILE *fp, const FileMetadata *files, int file_count)
{
    ZipCentralDirHeader *central_dir = NULL;
    int central_dir_count = 0;
    uint32_t central_dir_size = 0;
    uint32_t local_header_offset = 0;

    // 分配中央目录数组
    central_dir = (ZipCentralDirHeader *)malloc(file_count * sizeof(ZipCentralDirHeader));
    if (central_dir == NULL)
    {
        return BACKUP_ERROR_MEMORY;
    }

    // 处理每个文件
    for (int i = 0; i < file_count; i++)
    {
        FILE *src_fp = fopen(files[i].path, "rb");
        if (src_fp == NULL)
        {
            free(central_dir);
            return BACKUP_ERROR_FILE;
        }

        // 获取文件名（仅文件名，不包含路径）
        const char *filename = strrchr(files[i].name, '\\');
        if (filename == NULL)
        {
            filename = files[i].name;
        }
        else
        {
            filename++;
        }

        uint16_t filename_length = (uint16_t)strlen(filename);
        uint32_t file_size = (uint32_t)files[i].size;

        // 计算文件CRC32
        uint32_t crc32 = 0;
        char buffer[4096];
        size_t bytes_read;

        fseek(src_fp, 0, SEEK_SET);
        while ((bytes_read = fread(buffer, 1, sizeof(buffer), src_fp)) > 0)
        {
            crc32 = calculate_crc32(buffer, bytes_read);
        }

        fseek(src_fp, 0, SEEK_SET);

        // 转换时间格式
        uint16_t zip_date, zip_time;
        convert_time_to_zip_format(files[i].modify_time, &zip_date, &zip_time);

        // 创建并写入本地文件头
        ZipLocalFileHeader local_header;
        memset(&local_header, 0, sizeof(local_header));
        local_header.signature = ZIP_LOCAL_FILE_HEADER_SIGNATURE;
        local_header.version_needed = 20; // 2.0版本
        local_header.flags = 0;
        local_header.compression_method = ZIP_COMPRESSION_METHOD_NONE;
        local_header.last_mod_time = zip_time;
        local_header.last_mod_date = zip_date;
        local_header.crc32 = crc32;
        local_header.compressed_size = file_size;
        local_header.uncompressed_size = file_size;
        local_header.filename_length = filename_length;
        local_header.extra_field_length = 0;

        if (fwrite(&local_header, sizeof(local_header), 1, fp) != 1)
        {
            fclose(src_fp);
            free(central_dir);
            return BACKUP_ERROR_FILE;
        }

        // 写入文件名
        if (fwrite(filename, 1, filename_length, fp) != filename_length)
        {
            fclose(src_fp);
            free(central_dir);
            return BACKUP_ERROR_FILE;
        }

        // 写入文件数据
        fseek(src_fp, 0, SEEK_SET);
        while ((bytes_read = fread(buffer, 1, sizeof(buffer), src_fp)) > 0)
        {
            if (fwrite(buffer, 1, bytes_read, fp) != bytes_read)
            {
                fclose(src_fp);
                free(central_dir);
                return BACKUP_ERROR_FILE;
            }
        }

        fclose(src_fp);

        // 添加到中央目录
        ZipCentralDirHeader *cd_header = &central_dir[central_dir_count++];
        memset(cd_header, 0, sizeof(*cd_header));
        cd_header->signature = ZIP_CENTRAL_DIR_HEADER_SIGNATURE;
        cd_header->created_version = 20; // 2.0版本
        cd_header->version_needed = 20;
        cd_header->flags = 0;
        cd_header->compression_method = ZIP_COMPRESSION_METHOD_NONE;
        cd_header->last_mod_time = zip_time;
        cd_header->last_mod_date = zip_date;
        cd_header->crc32 = crc32;
        cd_header->compressed_size = file_size;
        cd_header->uncompressed_size = file_size;
        cd_header->filename_length = filename_length;
        cd_header->extra_field_length = 0;
        cd_header->file_comment_length = 0;
        cd_header->disk_number = 0;
        cd_header->internal_attrs = 0x0000;
        cd_header->external_attrs = 0x81A40000; // 普通文件
        cd_header->local_header_offset = local_header_offset;

        // 计算下一个本地文件头的偏移
        local_header_offset += sizeof(local_header) + filename_length + file_size;
    }

    // 保存中央目录偏移
    uint32_t central_dir_offset = local_header_offset;

    // 写入中央目录
    for (int i = 0; i < central_dir_count; i++)
    {
        // 写入中央目录头
        if (fwrite(&central_dir[i], sizeof(ZipCentralDirHeader), 1, fp) != 1)
        {
            free(central_dir);
            return BACKUP_ERROR_FILE;
        }

        // 写入文件名
        const char *filename = strrchr(files[i].name, '\\');
        if (filename == NULL)
        {
            filename = files[i].name;
        }
        else
        {
            filename++;
        }

        if (fwrite(filename, 1, central_dir[i].filename_length, fp) != central_dir[i].filename_length)
        {
            free(central_dir);
            return BACKUP_ERROR_FILE;
        }

        central_dir_size += sizeof(ZipCentralDirHeader) + central_dir[i].filename_length;
    }

    // 写入中央目录结束记录
    ZipEndCentralDir end_cd;
    memset(&end_cd, 0, sizeof(end_cd));
    end_cd.signature = ZIP_END_CENTRAL_DIR_SIGNATURE;
    end_cd.disk_number = 0;
    end_cd.start_disk_number = 0;
    end_cd.entries_on_disk = (uint16_t)central_dir_count;
    end_cd.total_entries = (uint16_t)central_dir_count;
    end_cd.central_dir_size = central_dir_size;
    end_cd.central_dir_offset = central_dir_offset;
    end_cd.comment_length = 0;

    if (fwrite(&end_cd, sizeof(end_cd), 1, fp) != 1)
    {
        free(central_dir);
        return BACKUP_ERROR_FILE;
    }

    free(central_dir);
    return BACKUP_SUCCESS;
}

// 调用外部命令打包文件
BackupResult external_pack(const char *output_path, const FileMetadata *files, int file_count, const char *algorithm)
{
    char command[1024] = {0};
    char temp_list_file[256] = {0};
    FILE *list_fp = NULL;
    int i;

    // 创建临时文件列表
    sprintf(temp_list_file, "%s_list.txt", output_path);
    list_fp = fopen(temp_list_file, "w");
    if (list_fp == NULL)
    {
        return BACKUP_ERROR_FILE;
    }

    // 写入文件列表
    for (i = 0; i < file_count; i++)
    {
        fprintf(list_fp, "%s\n", files[i].path);
    }
    fclose(list_fp);

    // 构建打包命令
    if (strcmp(algorithm, "zip") == 0)
    {
        // 使用zip命令
        sprintf(command, "zip -r %s @%s", output_path, temp_list_file);
    }
    else if (strcmp(algorithm, "7z") == 0)
    {
        // 使用7z命令
        sprintf(command, "7z a %s @%s", output_path, temp_list_file);
    }
    else
    {
        // 无效算法
        remove(temp_list_file);
        return BACKUP_ERROR_PARAM;
    }

    // 执行命令
    int result = system(command);

    // 删除临时文件
    remove(temp_list_file);

    if (result != 0)
    {
        return BACKUP_ERROR_PACK;
    }

    return BACKUP_SUCCESS;
}

// 打包文件
BackupResult pack_files(const char *output_path, const FileMetadata *files, int file_count, PackAlgorithm algorithm, CompressAlgorithm compress_algorithm)
{
    printf("DEBUG: Entering pack_files function\n");
    printf("DEBUG: Output path: %s\n", output_path);
    printf("DEBUG: File count: %d\n", file_count);
    printf("DEBUG: Pack algorithm: %d\n", algorithm);
    printf("DEBUG: Compress algorithm: %d\n", compress_algorithm);

    FILE *fp = NULL;
    BackupResult result = BACKUP_SUCCESS;

    // 检查参数
    if (output_path == NULL || files == NULL || file_count <= 0)
    {
        printf("DEBUG: Invalid parameters\n");
        return BACKUP_ERROR_PARAM;
    }

    // 根据算法选择打包方式
    switch (algorithm)
    {
    case PACK_ALGORITHM_TAR:
    case PACK_ALGORITHM_ZIP:
        // 打开输出文件
        printf("DEBUG: Opening output file for writing\n");
        fp = fopen(output_path, "wb");
        if (fp == NULL)
        {
            printf("DEBUG: Failed to open output file, error: %d\n", errno);
            return BACKUP_ERROR_FILE;
        }

        if (algorithm == PACK_ALGORITHM_TAR)
        {
            result = tar_pack(fp, files, file_count);
        }
        else
        {
            result = zip_pack(fp, files, file_count);
        }
        fclose(fp);
        break;
    default:
        printf("DEBUG: Invalid algorithm\n");
        result = BACKUP_ERROR_PARAM;
        break;
    }

    return result;
}

// Tar解包实现
BackupResult tar_unpack(FILE *fp, FileMetadata **files, int *file_count)
{
    // 简化的Tar解包实现
    char header[512];
    size_t bytes_read;
    int count = 0;
    int max_files = 100;

    // 分配初始文件元数据数组
    *files = (FileMetadata *)malloc(max_files * sizeof(FileMetadata));
    if (*files == NULL)
    {
        return BACKUP_ERROR_MEMORY;
    }

    while (1)
    {
        // 读取Tar文件头
        bytes_read = fread(header, 1, 512, fp);
        if (bytes_read != 512)
        {
            // 检查是否遇到文件结束
            if (feof(fp))
            {
                break;
            }
            else
            {
                free(*files);
                return BACKUP_ERROR_FILE;
            }
        }

        // 检查文件头是否全零（结束块）
        int all_zero = 1;
        for (int i = 0; i < 512; i++)
        {
            if (header[i] != 0)
            {
                all_zero = 0;
                break;
            }
        }
        if (all_zero)
        {
            // 再读一个块，确认是否是结束标记（两个连续的全零块）
            bytes_read = fread(header, 1, 512, fp);
            if (bytes_read == 512)
            {
                // 检查第二个块是否也是全零
                int second_all_zero = 1;
                for (int i = 0; i < 512; i++)
                {
                    if (header[i] != 0)
                    {
                        second_all_zero = 0;
                        break;
                    }
                }
                if (second_all_zero)
                {
                    // 找到结束标记，退出循环
                    break;
                }
                else
                {
                    // 不是结束标记，将文件指针移回
                    fseek(fp, -512, SEEK_CUR);
                }
            }
            else
            {
                // 读取失败，退出循环
                break;
            }
        }

        // 解析文件名
        char filename[256] = {0};
        strncpy(filename, header, 100);

        // 跳过空文件名（可能是填充块）
        if (filename[0] == '\0')
        {
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
        if (count >= max_files)
        {
            max_files *= 2;
            FileMetadata *new_files = (FileMetadata *)realloc(*files, max_files * sizeof(FileMetadata));
            if (new_files == NULL)
            {
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

        // 创建文件所在的目录结构
        char dir_path[256];
        strcpy(dir_path, filename);
        char *last_slash = strrchr(dir_path, '\\');
        if (last_slash != NULL)
        {
            *last_slash = '\0';
            // 递归创建目录
            create_directory_recursive(dir_path);
        }

        // 写入文件数据
        FILE *dst_fp = fopen(filename, "wb");
        if (dst_fp == NULL)
        {
            free(*files);
            return BACKUP_ERROR_FILE;
        }

        char buffer[4096];
        size_t bytes_written;
        size_t remaining = size;
        while (remaining > 0)
        {
            bytes_read = fread(buffer, 1, (remaining < sizeof(buffer)) ? remaining : sizeof(buffer), fp);
            if (bytes_read == 0)
            {
                fclose(dst_fp);
                free(*files);
                return BACKUP_ERROR_FILE;
            }

            bytes_written = fwrite(buffer, 1, bytes_read, dst_fp);
            if (bytes_written != bytes_read)
            {
                fclose(dst_fp);
                free(*files);
                return BACKUP_ERROR_FILE;
            }

            remaining -= bytes_written;
        }

        fclose(dst_fp);

        // 跳过文件数据后的填充
        if (size % 512 != 0)
        {
            size_t padding = 512 - (size % 512);
            if (fseek(fp, padding, SEEK_CUR) != 0)
            {
                free(*files);
                return BACKUP_ERROR_FILE;
            }
        }

        count++;
    }

    // 调整数组大小
    *files = (FileMetadata *)realloc(*files, count * sizeof(FileMetadata));
    if (*files == NULL && count > 0)
    {
        return BACKUP_ERROR_MEMORY;
    }

    *file_count = count;
    return BACKUP_SUCCESS;
}

// 调用外部命令解包文件
BackupResult external_unpack(const char *input_path, FileMetadata **files, int *file_count, const char *algorithm)
{
    char command[1024] = {0};

    // 构建解包命令
    if (strcmp(algorithm, "zip") == 0)
    {
        // 使用unzip命令
        sprintf(command, "unzip %s -d .", input_path);
    }
    else if (strcmp(algorithm, "7z") == 0)
    {
        // 使用7z命令
        sprintf(command, "7z x %s -o.", input_path);
    }
    else
    {
        // 无效算法
        return BACKUP_ERROR_PARAM;
    }

    // 执行命令
    int result = system(command);
    if (result != 0)
    {
        return BACKUP_ERROR_PACK;
    }

    // 由于是外部命令解包，我们无法直接获取文件元数据，这里返回空数组
    *files = NULL;
    *file_count = 0;

    return BACKUP_SUCCESS;
}

// 解包文件
BackupResult unpack_files(const char *input_path, FileMetadata **files, int *file_count, PackAlgorithm algorithm)
{
    printf("DEBUG: Entering unpack_files function\n");
    printf("DEBUG: Input path: %s\n", input_path);
    printf("DEBUG: Algorithm: %d\n", algorithm);

    FILE *fp = NULL;
    BackupResult result = BACKUP_SUCCESS;

    // 检查参数
    if (input_path == NULL || files == NULL || file_count == NULL)
    {
        printf("DEBUG: Invalid parameters\n");
        return BACKUP_ERROR_PARAM;
    }

    // 根据算法选择解包方式
    switch (algorithm)
    {
    case PACK_ALGORITHM_TAR:
    case PACK_ALGORITHM_ZIP:
        // 打开输入文件
        printf("DEBUG: Opening input file for reading\n");
        fp = fopen(input_path, "rb");
        if (fp == NULL)
        {
            printf("DEBUG: Failed to open input file, errno: %d\n", errno);
            return BACKUP_ERROR_FILE;
        }

        if (algorithm == PACK_ALGORITHM_TAR)
        {
            result = tar_unpack(fp, files, file_count);
        }
        else
        {
            result = zip_unpack(fp, files, file_count);
        }
        fclose(fp);
        break;
    default:
        printf("DEBUG: Invalid algorithm\n");
        result = BACKUP_ERROR_PARAM;
        break;
    }

    printf("DEBUG: Unpack files completed, result: %d, file count: %d\n", result, *file_count);
    return result;
}

// 写入打包文件头部
BackupResult write_pack_header(FILE *fp, const PackHeader *header)
{
    if (fp == NULL || header == NULL)
    {
        return BACKUP_ERROR_PARAM;
    }

    size_t bytes_written = fwrite(header, sizeof(PackHeader), 1, fp);
    if (bytes_written != 1)
    {
        return BACKUP_ERROR_FILE;
    }

    return BACKUP_SUCCESS;
}

// 读取打包文件头部
BackupResult read_pack_header(FILE *fp, PackHeader *header)
{
    if (fp == NULL || header == NULL)
    {
        return BACKUP_ERROR_PARAM;
    }

    size_t bytes_read = fread(header, sizeof(PackHeader), 1, fp);
    if (bytes_read != 1)
    {
        return BACKUP_ERROR_FILE;
    }

    return BACKUP_SUCCESS;
}

// 写入文件项
BackupResult write_pack_file_item(FILE *fp, const PackFileItem *item)
{
    if (fp == NULL || item == NULL)
    {
        return BACKUP_ERROR_PARAM;
    }

    size_t bytes_written = fwrite(item, sizeof(PackFileItem), 1, fp);
    if (bytes_written != 1)
    {
        return BACKUP_ERROR_FILE;
    }

    return BACKUP_SUCCESS;
}

// 读取文件项
BackupResult read_pack_file_item(FILE *fp, PackFileItem *item)
{
    if (fp == NULL || item == NULL)
    {
        return BACKUP_ERROR_PARAM;
    }

    size_t bytes_read = fread(item, sizeof(PackFileItem), 1, fp);
    if (bytes_read != 1)
    {
        return BACKUP_ERROR_FILE;
    }

    return BACKUP_SUCCESS;
}