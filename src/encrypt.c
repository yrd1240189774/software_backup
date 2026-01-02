#include "../include/encrypt.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

// 内部错误处理函数
static void handle_openssl_error(void) {
    ERR_print_errors_fp(stderr);
}

// 安全的密钥派生函数（使用PBKDF2）
BackupResult derive_key_pbkdf2(const char *password, const unsigned char *salt, 
                                    unsigned char *derived_key, uint32_t key_size) {
    if (!password || !salt || !derived_key || key_size != AES_256_KEY_SIZE) {
        return BACKUP_ERROR_PARAM;
    }

    // 使用PKCS5_PBKDF2_HMAC进行安全的密钥派生
    if (PKCS5_PBKDF2_HMAC(password, strlen(password), salt, 16, 
                         PBKDF2_ITERATIONS, EVP_sha256(), key_size, derived_key) != 1) {
        handle_openssl_error();
        return BACKUP_ERROR_ENCRYPT;
    }
    
    return BACKUP_SUCCESS;
}

// 文件头写入函数（确保跨平台兼容性）
BackupResult write_encrypt_header(FILE *fp, const EncryptHeader *header) {
    if (!fp || !header) return BACKUP_ERROR_PARAM;

    // 分别写入每个字段，避免结构体对齐问题
    if (fwrite(header->magic, 1, 4, fp) != 4) return BACKUP_ERROR_FILE;
    if (fwrite(&header->version, sizeof(header->version), 1, fp) != 1) return BACKUP_ERROR_FILE;
    if (fwrite(&header->algorithm, sizeof(header->algorithm), 1, fp) != 1) return BACKUP_ERROR_FILE;
    if (fwrite(&header->original_size, sizeof(header->original_size), 1, fp) != 1) return BACKUP_ERROR_FILE;
    if (fwrite(header->salt, 1, 16, fp) != 16) return BACKUP_ERROR_FILE;
    if (fwrite(header->iv, 1, 16, fp) != 16) return BACKUP_ERROR_FILE;
    if (fwrite(&header->iterations, sizeof(header->iterations), 1, fp) != 1) return BACKUP_ERROR_FILE;

    return BACKUP_SUCCESS;
}

// 文件头读取函数
BackupResult read_encrypt_header(FILE *fp, EncryptHeader *header) {
    if (!fp || !header) return BACKUP_ERROR_PARAM;

    if (fread(header->magic, 1, 4, fp) != 4) return BACKUP_ERROR_FILE;
    if (fread(&header->version, sizeof(header->version), 1, fp) != 1) return BACKUP_ERROR_FILE;
    if (fread(&header->algorithm, sizeof(header->algorithm), 1, fp) != 1) return BACKUP_ERROR_FILE;
    if (fread(&header->original_size, sizeof(header->original_size), 1, fp) != 1) return BACKUP_ERROR_FILE;
    if (fread(header->salt, 1, 16, fp) != 16) return BACKUP_ERROR_FILE;
    if (fread(header->iv, 1, 16, fp) != 16) return BACKUP_ERROR_FILE;
    if (fread(&header->iterations, sizeof(header->iterations), 1, fp) != 1) return BACKUP_ERROR_FILE;

    return BACKUP_SUCCESS;
}

// 核心加密函数
BackupResult encrypt_file(const char *input_path, const char *output_path, const char *password) {
    // 参数验证
    if (!input_path || !output_path || !password || strlen(password) == 0) {
        return BACKUP_ERROR_PARAM;
    }

    FILE *input_fp = NULL, *output_fp = NULL;
    EVP_CIPHER_CTX *ctx = NULL;
    BackupResult result = BACKUP_SUCCESS;
    EncryptHeader header = {0};

    // 1. 打开文件
    input_fp = fopen(input_path, "rb");
    if (!input_fp) {
        result = BACKUP_ERROR_FILE;
        goto cleanup;
    }

    output_fp = fopen(output_path, "wb");
    if (!output_fp) {
        result = BACKUP_ERROR_FILE;
        goto cleanup;
    }

    // 2. 获取文件大小
    fseek(input_fp, 0, SEEK_END);
    long file_size = ftell(input_fp);
    fseek(input_fp, 0, SEEK_SET);

    if (file_size <= 0) {
        result = BACKUP_ERROR_NO_FILES;
        goto cleanup;
    }

    // 3. 初始化加密头
    memcpy(header.magic, "ENCR", 4);
    header.version = 1;
    header.algorithm = ENCRYPT_ALGORITHM_AES;
    header.original_size = (unsigned long)file_size;
    header.iterations = PBKDF2_ITERATIONS;

    // 4. 生成密码学安全的随机盐和IV
    if (RAND_bytes(header.salt, 16) != 1 || RAND_bytes(header.iv, 16) != 1) {
        handle_openssl_error();
        result = BACKUP_ERROR_ENCRYPT;
        goto cleanup;
    }

    // 5. 写入文件头
    if (write_encrypt_header(output_fp, &header) != BACKUP_SUCCESS) {
        result = BACKUP_ERROR_FILE;
        goto cleanup;
    }

    // 6. 派生加密密钥
    unsigned char derived_key[AES_256_KEY_SIZE] = {0};
    if (derive_key_pbkdf2(password, header.salt, derived_key, AES_256_KEY_SIZE) != BACKUP_SUCCESS) {
        result = BACKUP_ERROR_ENCRYPT;
        goto cleanup;
    }

    // 7. 初始化加密上下文
    ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        result = BACKUP_ERROR_MEMORY;
        goto cleanup;
    }

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, derived_key, header.iv) != 1) {
        handle_openssl_error();
        result = BACKUP_ERROR_ENCRYPT;
        goto cleanup;
    }

    // 8. 流式加密处理
    unsigned char in_buf[BUFFER_SIZE], out_buf[BUFFER_SIZE + AES_BLOCK_SIZE];
    int bytes_read, out_len;

    while ((bytes_read = fread(in_buf, 1, BUFFER_SIZE, input_fp)) > 0) {
        if (EVP_EncryptUpdate(ctx, out_buf, &out_len, in_buf, bytes_read) != 1) {
            handle_openssl_error();
            result = BACKUP_ERROR_ENCRYPT;
            goto cleanup;
        }
        if (fwrite(out_buf, 1, out_len, output_fp) != (size_t)out_len) {
            result = BACKUP_ERROR_FILE;
            goto cleanup;
        }
    }

    // 9. 处理最终块
    if (EVP_EncryptFinal_ex(ctx, out_buf, &out_len) != 1) {
        handle_openssl_error();
        result = BACKUP_ERROR_ENCRYPT;
        goto cleanup;
    }
    if (out_len > 0 && fwrite(out_buf, 1, out_len, output_fp) != (size_t)out_len) {
        result = BACKUP_ERROR_FILE;
        goto cleanup;
    }

cleanup:
    // 10. 资源清理
    if (ctx) {
        EVP_CIPHER_CTX_free(ctx);
        ctx = NULL;
    }
    if (input_fp) {
        fclose(input_fp);
        input_fp = NULL;
    }
    if (output_fp) {
        fclose(output_fp);
        output_fp = NULL;
    }
    // 无条件清空密钥内存
    memset(derived_key, 0, sizeof(derived_key));
    
    return result;
}

// 核心解密函数
BackupResult decrypt_file(const char *input_path, const char *output_path, const char *password) {
    if (!input_path || !output_path || !password) {
        return BACKUP_ERROR_PARAM;
    }

    FILE *input_fp = NULL, *output_fp = NULL;
    EVP_CIPHER_CTX *ctx = NULL;
    BackupResult result = BACKUP_SUCCESS;
    EncryptHeader header = {0};

    input_fp = fopen(input_path, "rb");
    if (!input_fp) return BACKUP_ERROR_FILE;

    output_fp = fopen(output_path, "wb");
    if (!output_fp) {
        fclose(input_fp);
        return BACKUP_ERROR_FILE;
    }

    // 1. 读取并验证文件头
    if (read_encrypt_header(input_fp, &header) != BACKUP_SUCCESS) {
        result = BACKUP_ERROR_FILE;
        goto cleanup;
    }

    if (memcmp(header.magic, "ENCR", 4) != 0) {
        result = BACKUP_ERROR_ENCRYPT;
        goto cleanup;
    }

    // 2. 派生解密密钥
    unsigned char derived_key[AES_256_KEY_SIZE] = {0};
    if (derive_key_pbkdf2(password, header.salt, derived_key, AES_256_KEY_SIZE) != BACKUP_SUCCESS) {
        result = BACKUP_ERROR_ENCRYPT;
        goto cleanup;
    }

    // 3. 初始化解密上下文
    ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        result = BACKUP_ERROR_MEMORY;
        goto cleanup;
    }

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, derived_key, header.iv) != 1) {
        handle_openssl_error();
        result = BACKUP_ERROR_ENCRYPT;
        goto cleanup;
    }

    // 4. 流式解密处理
    unsigned char in_buf[BUFFER_SIZE], out_buf[BUFFER_SIZE + AES_BLOCK_SIZE];
    int bytes_read, out_len;

    while ((bytes_read = fread(in_buf, 1, BUFFER_SIZE, input_fp)) > 0) {
        if (EVP_DecryptUpdate(ctx, out_buf, &out_len, in_buf, bytes_read) != 1) {
            handle_openssl_error();
            result = BACKUP_ERROR_ENCRYPT;
            goto cleanup;
        }
        if (fwrite(out_buf, 1, out_len, output_fp) != (size_t)out_len) {
            result = BACKUP_ERROR_FILE;
            goto cleanup;
        }
    }

    // 5. 处理最终块
    if (EVP_DecryptFinal_ex(ctx, out_buf, &out_len) != 1) {
        handle_openssl_error();
        result = BACKUP_ERROR_ENCRYPT; // 可能是密码错误
        goto cleanup;
    }
    if (out_len > 0 && fwrite(out_buf, 1, out_len, output_fp) != (size_t)out_len) {
        result = BACKUP_ERROR_FILE;
        goto cleanup;
    }

cleanup:
    if (ctx) EVP_CIPHER_CTX_free(ctx);
    if (input_fp) fclose(input_fp);
    if (output_fp) fclose(output_fp);
    if (derived_key[0]) memset(derived_key, 0, AES_256_KEY_SIZE);
    
    return result;
}