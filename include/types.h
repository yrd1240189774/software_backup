#ifndef TYPES_H
#define TYPES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// 文件类型枚举
typedef enum {
    FILE_TYPE_REGULAR = 0x01,
    FILE_TYPE_DIRECTORY = 0x02,
    FILE_TYPE_SYMLINK = 0x04,
    FILE_TYPE_BLOCK = 0x08,
    FILE_TYPE_CHARACTER = 0x10,
    FILE_TYPE_SOCKET = 0x20,
    FILE_TYPE_FIFO = 0x40,
    FILE_TYPE_ALL = 0x7F
} FileType;

// 打包算法枚举
typedef enum {
    PACK_ALGORITHM_MYPACK,
    PACK_ALGORITHM_TAR
} PackAlgorithm;

// 压缩算法枚举
typedef enum {
    COMPRESS_ALGORITHM_NONE,
    COMPRESS_ALGORITHM_HAFF,
    COMPRESS_ALGORITHM_LZ77
} CompressAlgorithm;

// 加密算法枚举
typedef enum {
    ENCRYPT_ALGORITHM_NONE,
    ENCRYPT_ALGORITHM_AES,
    ENCRYPT_ALGORITHM_DES
} EncryptAlgorithm;

// 文件元数据结构体
typedef struct {
    char path[256];            // 文件路径
    char name[256];            // 文件名
    FileType type;             // 文件类型
    unsigned long size;        // 文件大小
    time_t create_time;        // 创建时间
    time_t modify_time;        // 修改时间
    time_t access_time;        // 访问时间
    unsigned int mode;         // 文件权限
    unsigned int uid;          // 用户ID
    unsigned int gid;          // 组ID
    char symlink_target[256];  // 符号链接目标（如果是符号链接）
} FileMetadata;

// 时间范围结构体
typedef struct {
    int enable;                // 是否启用时间筛选
    time_t start_time;         // 开始时间
    time_t end_time;           // 结束时间
} TimeRange;

// 尺寸范围结构体
typedef struct {
    unsigned long min_size;    // 最小尺寸
    unsigned long max_size;    // 最大尺寸
} SizeRange;

// 排除用户/组结构体
typedef struct {
    int enable_user;           // 是否启用排除用户
    int enable_group;          // 是否启用排除组
    char *exclude_users[100];  // 排除的用户列表
    char *exclude_groups[100]; // 排除的组列表
    int user_count;            // 用户数量
    int group_count;           // 组数量
} ExcludeUserGroup;

// 排除文件名结构体
typedef struct {
    char pattern[256];         // 排除的文件名模式（正则表达式）
} ExcludeFilename;

// 排除目录结构体
typedef struct {
    char paths[100][256];      // 排除的目录列表
    int count;                 // 目录数量
} ExcludeDirectory;

// 备份选项结构体
typedef struct {
    char source_path[256];     // 源路径
    char target_path[256];     // 目标路径
    FileType file_types;       // 备份的文件类型
    
    // 时间筛选
    TimeRange create_time_range;
    TimeRange modify_time_range;
    TimeRange access_time_range;
    
    // 尺寸筛选
    SizeRange size_range;
    
    // 排除选项
    ExcludeUserGroup exclude_usergroup;
    ExcludeFilename exclude_filename;
    ExcludeDirectory exclude_directory;
    
    // 打包选项
    PackAlgorithm pack_algorithm;
    
    // 压缩选项
    CompressAlgorithm compress_algorithm;
    
    // 加密选项
    int encrypt_enable;        // 是否启用加密
    EncryptAlgorithm encrypt_algorithm;
    char encrypt_key[64];      // 加密密钥
} BackupOptions;

// 还原选项结构体
typedef struct {
    char backup_file[256];     // 备份文件路径
    char target_path[256];     // 还原目标路径
    
    // 加密选项
    int encrypt_enable;        // 是否启用解密
    EncryptAlgorithm encrypt_algorithm;
    char encrypt_key[64];      // 解密密钥
} RestoreOptions;

// 函数返回值枚举
typedef enum {
    BACKUP_SUCCESS = 0,
    BACKUP_ERROR_PARAM = -1,
    BACKUP_ERROR_PATH = -2,
    BACKUP_ERROR_FILE = -3,
    BACKUP_ERROR_MEMORY = -4,
    BACKUP_ERROR_COMPRESS = -5,
    BACKUP_ERROR_ENCRYPT = -6,
    BACKUP_ERROR_PACK = -7,
    BACKUP_ERROR_NO_FILES = -8  // 没有文件需要备份
} BackupResult;

#endif // TYPES_H