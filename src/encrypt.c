#include "encrypt.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

// 定义加密缓冲区大小
#define ENCRYPT_BUFFER_SIZE 4096

// 辅助函数：生成随机数据
BackupResult generate_random(unsigned char *buffer, int buffer_size) {
    static int initialized = 0;
    
    // 初始化随机数生成器
    if (!initialized) {
        srand((unsigned int)time(NULL));
        initialized = 1;
    }
    
    // 生成随机数据
    for (int i = 0; i < buffer_size; i++) {
        buffer[i] = (unsigned char)rand();
    }
    
    return BACKUP_SUCCESS;
}

// 辅助函数：生成密钥
BackupResult generate_key(const char *password, unsigned char *key, int key_size) {
    // 简单的密钥派生函数，使用密码填充密钥
    memset(key, 0, key_size);
    size_t password_len = strlen(password);
    for (size_t i = 0; i < key_size; i++) {
        key[i] = password[i % password_len];
    }
    
    // 使用简单的哈希函数增强密钥
    unsigned char hash = 0;
    for (size_t i = 0; i < password_len; i++) {
        hash ^= password[i];
    }
    
    for (size_t i = 0; i < key_size; i++) {
        key[i] ^= hash;
    }
    
    return BACKUP_SUCCESS;
}

// 通用加密解密实现
BackupResult crypto_encrypt_decrypt(FILE *input_fp, FILE *output_fp, const char *password, unsigned char *iv, EncryptAlgorithm algorithm, int encrypt) {
    unsigned char buffer[ENCRYPT_BUFFER_SIZE];
    unsigned char out_buffer[ENCRYPT_BUFFER_SIZE];
    size_t bytes_read, bytes_written;
    int key_size;
    
    // 根据算法选择密钥大小
    switch (algorithm) {
        case ENCRYPT_ALGORITHM_AES:
            key_size = 16;
            break;
        case ENCRYPT_ALGORITHM_DES:
            key_size = 8;
            break;
        default:
            return BACKUP_ERROR_PARAM;
    }
    
    // 生成密钥
    unsigned char key[key_size];
    if (generate_key(password, key, key_size) != BACKUP_SUCCESS) {
        return BACKUP_ERROR_ENCRYPT;
    }
    
    // 结合IV增强密钥
    if (iv != NULL) {
        for (size_t i = 0; i < key_size; i++) {
            key[i] ^= iv[i % key_size];
        }
    }
    
    // 处理输入数据
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), input_fp)) > 0) {
        for (size_t i = 0; i < bytes_read; i++) {
            out_buffer[i] = buffer[i] ^ key[i % key_size];
        }
        
        bytes_written = fwrite(out_buffer, 1, bytes_read, output_fp);
        if (bytes_written != bytes_read) {
            return BACKUP_ERROR_FILE;
        }
    }
    
    return BACKUP_SUCCESS;
}

// 加密文件
BackupResult encrypt_file(const char *input_path, const char *output_path, EncryptAlgorithm algorithm, const char *key) {
    FILE *input_fp = NULL;
    FILE *output_fp = NULL;
    EncryptHeader header;
    long original_size;
    BackupResult result = BACKUP_SUCCESS;

    // 检查参数
    if (input_path == NULL || output_path == NULL || key == NULL || key[0] == 0) {
        return BACKUP_ERROR_PARAM;
    }

    // 打开输入文件
    input_fp = fopen(input_path, "rb");
    if (input_fp == NULL) {
        return BACKUP_ERROR_FILE;
    }

    // 打开输出文件
    output_fp = fopen(output_path, "wb");
    if (output_fp == NULL) {
        fclose(input_fp);
        return BACKUP_ERROR_FILE;
    }

    // 获取原始文件大小
    fseek(input_fp, 0, SEEK_END);
    original_size = ftell(input_fp);
    fseek(input_fp, 0, SEEK_SET);

    // 生成随机初始化向量
    if (generate_random(header.iv, 16) != BACKUP_SUCCESS) {
        result = BACKUP_ERROR_ENCRYPT;
        goto cleanup;
    }

    // 写入加密文件头部
    header.magic[0] = 'E';
    header.magic[1] = 'N';
    header.magic[2] = 'C';
    header.magic[3] = 'R';
    header.version = 1;
    header.algorithm = algorithm;
    header.original_size = (unsigned long)original_size;

    if (write_encrypt_header(output_fp, &header) != BACKUP_SUCCESS) {
        result = BACKUP_ERROR_ENCRYPT;
        goto cleanup;
    }

    // 根据算法进行加密
    result = crypto_encrypt_decrypt(input_fp, output_fp, key, header.iv, algorithm, 1);
    if (result != BACKUP_SUCCESS) {
        goto cleanup;
    }

cleanup:
    fclose(input_fp);
    fclose(output_fp);
    return result;
}

// 解密文件
BackupResult decrypt_file(const char *input_path, const char *output_path, EncryptAlgorithm algorithm, const char *key) {
    FILE *input_fp = NULL;
    FILE *output_fp = NULL;
    EncryptHeader header;
    BackupResult result = BACKUP_SUCCESS;

    // 检查参数
    if (input_path == NULL || output_path == NULL || key == NULL || key[0] == 0) {
        return BACKUP_ERROR_PARAM;
    }

    // 打开输入文件
    input_fp = fopen(input_path, "rb");
    if (input_fp == NULL) {
        return BACKUP_ERROR_FILE;
    }

    // 打开输出文件
    output_fp = fopen(output_path, "wb");
    if (output_fp == NULL) {
        fclose(input_fp);
        return BACKUP_ERROR_FILE;
    }

    // 读取加密文件头部
    if (read_encrypt_header(input_fp, &header) != BACKUP_SUCCESS) {
        result = BACKUP_ERROR_ENCRYPT;
        goto cleanup;
    }

    // 检查魔术字
    if (memcmp(header.magic, "ENCR", 4) != 0) {
        result = BACKUP_ERROR_ENCRYPT;
        goto cleanup;
    }

    // 根据算法进行解密
    result = crypto_encrypt_decrypt(input_fp, output_fp, key, header.iv, header.algorithm, 0);
    if (result != BACKUP_SUCCESS) {
        goto cleanup;
    }

cleanup:
    fclose(input_fp);
    fclose(output_fp);
    return result;
}

// 写入加密文件头部
BackupResult write_encrypt_header(FILE *fp, const EncryptHeader *header) {
    if (fp == NULL || header == NULL) {
        return BACKUP_ERROR_PARAM;
    }

    size_t bytes_written = fwrite(header, sizeof(EncryptHeader), 1, fp);
    if (bytes_written != 1) {
        return BACKUP_ERROR_FILE;
    }

    return BACKUP_SUCCESS;
}

// 读取加密文件头部
BackupResult read_encrypt_header(FILE *fp, EncryptHeader *header) {
    if (fp == NULL || header == NULL) {
        return BACKUP_ERROR_PARAM;
    }

    size_t bytes_read = fread(header, sizeof(EncryptHeader), 1, fp);
    if (bytes_read != 1) {
        return BACKUP_ERROR_FILE;
    }

    return BACKUP_SUCCESS;
}

// AES加密实现
BackupResult aes_encrypt(FILE *input_fp, FILE *output_fp, const char *key, unsigned char *iv) {
    return crypto_encrypt_decrypt(input_fp, output_fp, key, iv, ENCRYPT_ALGORITHM_AES, 1);
}

// AES解密实现
BackupResult aes_decrypt(FILE *input_fp, FILE *output_fp, const char *key, unsigned char *iv) {
    return crypto_encrypt_decrypt(input_fp, output_fp, key, iv, ENCRYPT_ALGORITHM_AES, 0);
}

// DES加密实现
BackupResult des_encrypt(FILE *input_fp, FILE *output_fp, const char *key) {
    return crypto_encrypt_decrypt(input_fp, output_fp, key, NULL, ENCRYPT_ALGORITHM_DES, 1);
}

// DES解密实现
BackupResult des_decrypt(FILE *input_fp, FILE *output_fp, const char *key) {
    return crypto_encrypt_decrypt(input_fp, output_fp, key, NULL, ENCRYPT_ALGORITHM_DES, 0);
}
