#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compress.h"
#include "types.h"

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
    unsigned char window[LZ77_WINDOW_SIZE] = {0}; // 滑动窗口
    unsigned char lookahead[LZ77_LOOKAHEAD_SIZE]; // 前瞻缓冲区
    int window_pos = 0; // 窗口当前位置
    int lookahead_pos = 0; // 前瞻缓冲区当前位置
    int lookahead_size = 0; // 前瞻缓冲区中有效数据的大小
    size_t bytes_read, bytes_written;
    int i;
    
    // 初始化前瞻缓冲区
    lookahead_size = fread(lookahead, 1, LZ77_LOOKAHEAD_SIZE, input_fp);
    if (lookahead_size == 0) {
        return BACKUP_SUCCESS;
    }
    
    // 开始压缩
    while (lookahead_pos < lookahead_size) {
        unsigned short best_offset = 0; // 最佳匹配偏移量，使用unsigned short确保2字节
        int best_length = 0; // 最佳匹配长度
        
        // 在滑动窗口中搜索最佳匹配
        for (i = 0; i < window_pos; i++) {
            int match_len = 0; // 当前匹配长度
            
            // 比较滑动窗口和前瞻缓冲区
            while (match_len < lookahead_size - lookahead_pos && 
                   i + match_len < window_pos && 
                   window[i + match_len] == lookahead[lookahead_pos + match_len]) {
                match_len++;
            }
            
            // 更新最佳匹配
            if (match_len >= LZ77_MIN_MATCH_LENGTH && match_len > best_length) {
                best_offset = window_pos - i;
                best_length = match_len;
                
                // 如果找到最大可能匹配，提前结束搜索
                if (match_len == LZ77_MAX_MATCH_LENGTH) {
                    break;
                }
            }
        }
        
        if (best_length >= LZ77_MIN_MATCH_LENGTH) {
            // 写入匹配标记
            unsigned char token = 0x80 | best_length;
            bytes_written = fwrite(&token, 1, 1, output_fp);
            if (bytes_written != 1) return BACKUP_ERROR_FILE;
            
            // 写入偏移量，确保写入2字节
            bytes_written = fwrite(&best_offset, sizeof(unsigned short), 1, output_fp);
            if (bytes_written != 1) return BACKUP_ERROR_FILE;
            
            // 将匹配的数据从前瞻缓冲区复制到滑动窗口
            for (i = 0; i < best_length; i++) {
                if (window_pos < LZ77_WINDOW_SIZE) {
                    window[window_pos++] = lookahead[lookahead_pos + i];
                } else {
                    // 窗口已满，滑动窗口
                    memmove(window, window + 1, LZ77_WINDOW_SIZE - 1);
                    window[LZ77_WINDOW_SIZE - 1] = lookahead[lookahead_pos + i];
                }
            }
            
            // 更新前瞻缓冲区位置
            lookahead_pos += best_length;
        } else {
            // 写入字面量
            unsigned char c = lookahead[lookahead_pos];
            bytes_written = fwrite(&c, 1, 1, output_fp);
            if (bytes_written != 1) return BACKUP_ERROR_FILE;
            
            // 将字面量添加到滑动窗口
            if (window_pos < LZ77_WINDOW_SIZE) {
                window[window_pos++] = c;
            } else {
                // 窗口已满，滑动窗口
                memmove(window, window + 1, LZ77_WINDOW_SIZE - 1);
                window[LZ77_WINDOW_SIZE - 1] = c;
            }
            
            // 更新前瞻缓冲区位置
            lookahead_pos++;
        }
        
        // 重新填充前瞻缓冲区
        if (lookahead_pos > lookahead_size - LZ77_MAX_MATCH_LENGTH) {
            // 将未处理的数据移到缓冲区开头
            memmove(lookahead, lookahead + lookahead_pos, lookahead_size - lookahead_pos);
            
            // 读取更多数据
            bytes_read = fread(lookahead + lookahead_size - lookahead_pos, 1, lookahead_pos, input_fp);
            lookahead_size = (lookahead_size - lookahead_pos) + bytes_read;
            lookahead_pos = 0;
        }
    }
    
    return BACKUP_SUCCESS;
}

// LZ77解压实现
BackupResult lz77_decompress(FILE *input_fp, FILE *output_fp) {
    unsigned char window[LZ77_WINDOW_SIZE] = {0}; // 滑动窗口
    int window_pos = 0; // 窗口当前位置
    size_t bytes_read, bytes_written;
    
    while (1) {
        unsigned char token;
        
        // 读取标记字节
        bytes_read = fread(&token, 1, 1, input_fp);
        if (bytes_read != 1) {
            if (feof(input_fp)) {
                break;
            } else {
                return BACKUP_ERROR_COMPRESS;
            }
        }
        
        if ((token & 0x80) == 0) {
            // 字面量
            unsigned char c = token;
            
            // 输出字面量
            bytes_written = fwrite(&c, 1, 1, output_fp);
            if (bytes_written != 1) return BACKUP_ERROR_FILE;
            
            // 将字面量添加到滑动窗口
            if (window_pos < LZ77_WINDOW_SIZE) {
                window[window_pos++] = c;
            } else {
                // 窗口已满，滑动窗口
                memmove(window, window + 1, LZ77_WINDOW_SIZE - 1);
                window[LZ77_WINDOW_SIZE - 1] = c;
            }
        } else {
            // 匹配
            int length = token & 0x7F; // 匹配长度
            unsigned short offset; // 匹配偏移量，使用unsigned short确保2字节
            
            // 读取偏移量，确保读取2字节
            bytes_read = fread(&offset, sizeof(unsigned short), 1, input_fp);
            if (bytes_read != 1) {
                return BACKUP_ERROR_COMPRESS;
            }
            
            // 计算匹配起始位置
            int start_pos = window_pos - offset;
            if (start_pos < 0) {
                return BACKUP_ERROR_COMPRESS;
            }
            
            // 输出匹配数据
            for (int i = 0; i < length; i++) {
                unsigned char c = window[start_pos + i];
                
                bytes_written = fwrite(&c, 1, 1, output_fp);
                if (bytes_written != 1) return BACKUP_ERROR_FILE;
                
                // 将字符添加到滑动窗口
                if (window_pos < LZ77_WINDOW_SIZE) {
                    window[window_pos++] = c;
                } else {
                    // 窗口已满，滑动窗口
                    memmove(window, window + 1, LZ77_WINDOW_SIZE - 1);
                    window[LZ77_WINDOW_SIZE - 1] = c;
                }
            }
        }
    }
    
    return BACKUP_SUCCESS;
}
