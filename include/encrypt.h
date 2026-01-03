#ifndef ENCRYPT_H
#define ENCRYPT_H

#include "types.h"
#include <stdio.h>
#include <stdint.h>

// 加密文件头部结构体
typedef struct {
    char magic[4];           // 魔术字 "ENCR"
    uint32_t version;        // 版本号
    EncryptAlgorithm algorithm; // 加密算法
    uint32_t original_size;  // 原始文件大小
    unsigned char salt[16];   // PBKDF2盐值
    unsigned char iv[16];     // 初始化向量
    uint32_t iterations;     // PBKDF2迭代次数
} EncryptHeader;

// 算法相关常量定义
#define AES_256_KEY_SIZE 32      // AES-256密钥长度（字节）
#define AES_BLOCK_SIZE 16         // AES块大小（字节）
#define SALT_SIZE 16              // PBKDF2盐值长度
#define IV_SIZE 16                // 初始化向量长度
#define PBKDF2_ITERATIONS 100000  // PBKDF2迭代次数
#define BUFFER_SIZE (64 * 1024)   // 流处理缓冲区大小（64KB）

/**
 * @brief 写入加密文件头
 * @param fp 文件指针
 * @param header 文件头结构体指针
 * @return 执行结果状态码
 */
BackupResult write_encrypt_header(FILE *fp, const EncryptHeader *header);

/**
 * @brief 读取加密文件头
 * @param fp 文件指针
 * @param header 文件头结构体指针
 * @return 执行结果状态码
 */
BackupResult read_encrypt_header(FILE *fp, EncryptHeader *header);

// ============================================================================
// 密钥派生函数
// ============================================================================

/**
 * @brief 使用PBKDF2算法从密码派生密钥
 * @param password 用户密码
 * @param salt 盐值
 * @param salt_len 盐值长度
 * @param derived_key 派生出的密钥缓冲区
 * @param key_size 需要的密钥长度
 * @return 执行结果状态码
 */
static BackupResult derive_key_pbkdf2(const char *password, const unsigned char *salt, 
                                    unsigned char *derived_key, uint32_t key_size);

// ============================================================================
// 随机数生成函数
// ============================================================================

/**
 * @brief 生成密码学安全的随机数
 * @param buffer 输出缓冲区
 * @param buffer_size 需要的随机数长度
 * @return 执行结果状态码
 */
BackupResult generate_secure_random(unsigned char *buffer, size_t buffer_size);

// ============================================================================
// 内部使用的OpenSSL相关函数（供实现文件使用）
// ============================================================================

/**
 * @brief 处理OpenSSL错误信息
 * @note 内部使用，将错误输出到stderr
 */
static void handle_openssl_error(void);
/**
 * @brief AES加密文件
 * @param input_path 输入文件路径（明文）
 * @param output_path 输出文件路径（密文）
 * @param password 加密密码
 * @return 执行结果状态码
 */
BackupResult encrypt_file(const char *input_path, const char *output_path, const char *password);

/**
 * @brief AES解密文件
 * @param input_path 输入文件路径（密文）
 * @param output_path 输出文件路径（明文）
 * @param password 解密密码
 * @return 执行结果状态码
 */
BackupResult decrypt_file(const char *input_path, const char *output_path, const char *password);




#endif // ENCRYPT_H
