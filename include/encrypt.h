#ifndef ENCRYPT_H
#define ENCRYPT_H

#include "types.h"

// 加密文件头部结构体
typedef struct {
    char magic[4];             // 魔术字，用于识别加密文件
    unsigned int version;      // 版本号
    EncryptAlgorithm algorithm; // 加密算法
    unsigned long original_size; // 原始文件大小
    unsigned char iv[16];      // 初始化向量（用于AES等算法）
} EncryptHeader;

// 加密解密模块内部函数声明
BackupResult write_encrypt_header(FILE *fp, const EncryptHeader *header);
BackupResult read_encrypt_header(FILE *fp, EncryptHeader *header);

// AES加密相关函数
BackupResult aes_encrypt(FILE *input_fp, FILE *output_fp, const char *key, unsigned char *iv);
BackupResult aes_decrypt(FILE *input_fp, FILE *output_fp, const char *key, unsigned char *iv);

// DES加密相关函数
BackupResult des_encrypt(FILE *input_fp, FILE *output_fp, const char *key);
BackupResult des_decrypt(FILE *input_fp, FILE *output_fp, const char *key);

#endif // ENCRYPT_H
