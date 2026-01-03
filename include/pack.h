#ifndef PACK_H
#define PACK_H

#include "types.h"

// 打包文件头部结构体
typedef struct {
    char magic[4];             // 魔术字，用于识别打包文件
    unsigned int version;      // 版本号
    unsigned int file_count;   // 文件数量
    unsigned long header_size; // 头部大小
    unsigned long data_offset; // 数据偏移量
} PackHeader;

// 打包文件项结构体
typedef struct {
    char path[256];            // 文件路径
    char name[256];            // 文件名
    FileType type;             // 文件类型
    unsigned long size;        // 文件大小
    unsigned long offset;      // 文件数据在打包文件中的偏移量
    time_t create_time;        // 创建时间
    time_t modify_time;        // 修改时间
    time_t access_time;        // 访问时间
    unsigned int mode;         // 文件权限
    unsigned int uid;          // 用户ID
    unsigned int gid;          // 组ID
    char symlink_target[256];  // 符号链接目标
} PackFileItem;

// 打包解包函数声明
BackupResult pack_files(const char *output_path, const FileMetadata *files, int file_count, PackAlgorithm algorithm, CompressAlgorithm compress_algorithm);
BackupResult unpack_files(const char *input_path, FileMetadata **files, int *file_count, PackAlgorithm algorithm);

// 打包解包模块内部函数声明
BackupResult write_pack_header(FILE *fp, const PackHeader *header);
BackupResult read_pack_header(FILE *fp, PackHeader *header);
BackupResult write_pack_file_item(FILE *fp, const PackFileItem *item);
BackupResult read_pack_file_item(FILE *fp, PackFileItem *item);

// 7z打包解包函数声明
BackupResult sevenz_pack(FILE *fp, const FileMetadata *files, int file_count, CompressAlgorithm compress_algorithm);
BackupResult sevenz_unpack(FILE *fp, FileMetadata **files, int *file_count);

#endif // PACK_H
