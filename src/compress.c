#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/compress.h"
#include "../include/types.h"

// 压缩文件
BackupResult compress_file(const char *input_path, const char *output_path, CompressAlgorithm algorithm) {
    FILE *input_fp = NULL;
    FILE *output_fp = NULL;
    CompressHeader header;
    long original_size;
    long compressed_size;
    BackupResult result = BACKUP_SUCCESS;

    // 检查参数
    if (input_path == NULL || output_path == NULL) {
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

    // 写入压缩文件头部（初始版本）
    header.magic[0] = 'C';
    header.magic[1] = 'O';
    header.magic[2] = 'M';
    header.magic[3] = 'P';
    header.version = 1;
    header.algorithm = algorithm;
    header.original_size = (unsigned long)original_size;
    header.compressed_size = 0; // 后续更新

    if (write_compress_header(output_fp, &header) != BACKUP_SUCCESS) {
        result = BACKUP_ERROR_COMPRESS;
        goto cleanup;
    }

    // 根据算法进行压缩
    switch (algorithm) {
        case COMPRESS_ALGORITHM_HAFF:
            result = huffman_compress(input_fp, output_fp);
            break;
        case COMPRESS_ALGORITHM_LZ77:
            result = lz77_compress(input_fp, output_fp);
            break;
        default:
            {
                // 不压缩，直接复制文件
                char buffer[4096];
                size_t bytes_read, bytes_written;
                while ((bytes_read = fread(buffer, 1, sizeof(buffer), input_fp)) > 0) {
                    bytes_written = fwrite(buffer, 1, bytes_read, output_fp);
                    if (bytes_written != bytes_read) {
                        result = BACKUP_ERROR_FILE;
                        goto cleanup;
                    }
                }
            }
            break;
    }

    if (result != BACKUP_SUCCESS) {
        goto cleanup;
    }

    // 更新压缩后文件大小
    compressed_size = ftell(output_fp);
    header.compressed_size = (unsigned long)(compressed_size - sizeof(CompressHeader));

    // 重新写入压缩文件头部
    fseek(output_fp, 0, SEEK_SET);
    if (write_compress_header(output_fp, &header) != BACKUP_SUCCESS) {
        result = BACKUP_ERROR_COMPRESS;
        goto cleanup;
    }

cleanup:
    fclose(input_fp);
    fclose(output_fp);
    return result;
}

// 解压文件
BackupResult decompress_file(const char *input_path, const char *output_path, CompressAlgorithm algorithm) {
    FILE *input_fp = NULL;
    FILE *output_fp = NULL;
    CompressHeader header;
    BackupResult result = BACKUP_SUCCESS;

    // 检查参数
    if (input_path == NULL || output_path == NULL) {
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

    // 读取压缩文件头部
    if (read_compress_header(input_fp, &header) != BACKUP_SUCCESS) {
        result = BACKUP_ERROR_COMPRESS;
        goto cleanup;
    }

    // 验证魔术字
    if (memcmp(header.magic, "COMP", 4) != 0) {
        result = BACKUP_ERROR_COMPRESS;
        goto cleanup;
    }

    // 根据算法进行解压
    switch (header.algorithm) {
        case COMPRESS_ALGORITHM_HAFF:
            result = huffman_decompress(input_fp, output_fp);
            break;
        case COMPRESS_ALGORITHM_LZ77:
            result = lz77_decompress(input_fp, output_fp);
            break;
        default:
            {
                // 不压缩，直接复制文件
                char buffer[4096];
                size_t bytes_read, bytes_written;
                while ((bytes_read = fread(buffer, 1, sizeof(buffer), input_fp)) > 0) {
                    bytes_written = fwrite(buffer, 1, bytes_read, output_fp);
                    if (bytes_written != bytes_read) {
                        result = BACKUP_ERROR_FILE;
                        goto cleanup;
                    }
                }
            }
            break;
    }

cleanup:
    fclose(input_fp);
    fclose(output_fp);
    return result;
}

// 写入压缩文件头部
BackupResult write_compress_header(FILE *fp, const CompressHeader *header) {
    if (fp == NULL || header == NULL) {
        return BACKUP_ERROR_PARAM;
    }

    size_t bytes_written = fwrite(header, sizeof(CompressHeader), 1, fp);
    if (bytes_written != 1) {
        return BACKUP_ERROR_FILE;
    }

    return BACKUP_SUCCESS;
}

// 读取压缩文件头部
BackupResult read_compress_header(FILE *fp, CompressHeader *header) {
    if (fp == NULL || header == NULL) {
        return BACKUP_ERROR_PARAM;
    }

    size_t bytes_read = fread(header, sizeof(CompressHeader), 1, fp);
    if (bytes_read != 1) {
        return BACKUP_ERROR_FILE;
    }

    return BACKUP_SUCCESS;
}

// LZ77压缩算法的配置
#define LZ77_WINDOW_SIZE 4096   // 滑动窗口大小
#define LZ77_LOOKAHEAD_SIZE 18  // 前瞻缓冲区大小
#define LZ77_MIN_MATCH_LENGTH 3  // 最小匹配长度
#define LZ77_MAX_MATCH_LENGTH 18 // 最大匹配长度

// LZ77压缩实现
BackupResult lz77_compress(FILE *input_fp, FILE *output_fp) {
    unsigned char *buffer = NULL;
    long file_size, i;
    int result = BACKUP_SUCCESS;
    
    // 获取文件大小
    fseek(input_fp, 0, SEEK_END);
    file_size = ftell(input_fp);
    fseek(input_fp, 0, SEEK_SET);
    
    // 分配内存
    buffer = (unsigned char *)malloc(file_size);
    if (buffer == NULL) {
        return BACKUP_ERROR_MEMORY;
    }
    
    // 读取整个文件
    if (fread(buffer, 1, file_size, input_fp) != file_size) {
        free(buffer);
        return BACKUP_ERROR_FILE;
    }
    
    long pos = 0;
    while (pos < file_size) {
        // 搜索匹配
        long best_length = 0;
        long best_offset = 0;
        long start_pos = (pos > LZ77_WINDOW_SIZE) ? pos - LZ77_WINDOW_SIZE : 0;
        
        for (long i = start_pos; i < pos; i++) {
            long match_length = 0;
            while (pos + match_length < file_size && 
                   i + match_length < pos && 
                   buffer[i + match_length] == buffer[pos + match_length] &&
                   match_length < LZ77_MAX_MATCH_LENGTH) {
                match_length++;
            }
            
            if (match_length >= LZ77_MIN_MATCH_LENGTH && match_length > best_length) {
                best_length = match_length;
                best_offset = pos - i;
            }
        }
        
        if (best_length >= LZ77_MIN_MATCH_LENGTH) {
            // 写入匹配标记 (最高位为1表示匹配)
            unsigned char token = 0x80 | (best_length & 0x7F);
            if (fwrite(&token, 1, 1, output_fp) != 1) {
                result = BACKUP_ERROR_FILE;
                break;
            }
            
            // 写入偏移量 (2字节)
            unsigned short offset = (unsigned short)best_offset;
            unsigned char offset_bytes[2];
            offset_bytes[0] = offset & 0xFF;
            offset_bytes[1] = (offset >> 8) & 0xFF;
            if (fwrite(offset_bytes, 1, 2, output_fp) != 2) {
                result = BACKUP_ERROR_FILE;
                break;
            }
            
            pos += best_length;
        } else {
            // 写入字面量 (最高位为0表示字面量)
            if (fwrite(&buffer[pos], 1, 1, output_fp) != 1) {
                result = BACKUP_ERROR_FILE;
                break;
            }
            pos++;
        }
    }
    
    free(buffer);
    return result;
}

// LZ77解压实现
BackupResult lz77_decompress(FILE *input_fp, FILE *output_fp) {
    unsigned char window[LZ77_WINDOW_SIZE];
    int window_pos = 0;
    int window_size = 0;
    BackupResult result = BACKUP_SUCCESS;
    
    // 初始化窗口
    memset(window, 0, LZ77_WINDOW_SIZE);
    
    while (1) {
        unsigned char token;
        size_t bytes_read = fread(&token, 1, 1, input_fp);
        if (bytes_read == 0) {
            if (feof(input_fp)) {
                break; // 文件结束
            } else {
                result = BACKUP_ERROR_COMPRESS;
                break;
            }
        }
        
        if ((token & 0x80) == 0) {
            // 字面量 (最高位为0)
            if (fwrite(&token, 1, 1, output_fp) != 1) {
                result = BACKUP_ERROR_FILE;
                break;
            }
            
            // 更新滑动窗口
            window[window_pos] = token;
            window_pos = (window_pos + 1) % LZ77_WINDOW_SIZE;
            if (window_size < LZ77_WINDOW_SIZE) {
                window_size++;
            }
        } else {
            // 匹配 (最高位为1)
            int length = token & 0x7F; // 取低7位作为长度
            
            // 读取2字节的偏移量
            unsigned char offset_bytes[2];
            bytes_read = fread(offset_bytes, 1, 2, input_fp);
            if (bytes_read != 2) {
                result = BACKUP_ERROR_COMPRESS;
                break;
            }
            
            unsigned short offset = offset_bytes[0] | (offset_bytes[1] << 8);
            
            // 检查偏移量是否有效
            if (offset > window_size || offset > LZ77_WINDOW_SIZE) {
                result = BACKUP_ERROR_COMPRESS;
                break;
            }
            
            // 计算匹配位置
            int match_pos = (window_pos - offset + LZ77_WINDOW_SIZE) % LZ77_WINDOW_SIZE;
            
            // 输出匹配的数据
            for (int i = 0; i < length; i++) {
                unsigned char c = window[match_pos];
                if (fwrite(&c, 1, 1, output_fp) != 1) {
                    result = BACKUP_ERROR_FILE;
                    break;
                }
                
                // 更新滑动窗口
                window[window_pos] = c;
                window_pos = (window_pos + 1) % LZ77_WINDOW_SIZE;
                if (window_size < LZ77_WINDOW_SIZE) {
                    window_size++;
                }
                
                match_pos = (match_pos + 1) % LZ77_WINDOW_SIZE;
            }
            
            if (result != BACKUP_SUCCESS) {
                break;
            }
        }
    }
    
    return result;
}