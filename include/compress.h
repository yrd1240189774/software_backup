#ifndef COMPRESS_H
#define COMPRESS_H

#include "types.h"

// 压缩文件头部结构体
typedef struct {
    char magic[4];             // 魔术字，用于识别压缩文件
    unsigned int version;      // 版本号
    CompressAlgorithm algorithm; // 压缩算法
    unsigned long original_size; // 原始文件大小
    unsigned long compressed_size; // 压缩后文件大小
} CompressHeader;

// Huffman树节点结构体
typedef struct HuffmanNode {
    unsigned char data;        // 字符数据
    unsigned long frequency;   // 频率
    struct HuffmanNode *left;  // 左子节点
    struct HuffmanNode *right; // 右子节点
} HuffmanNode;

// 压缩解压模块内部函数声明
BackupResult write_compress_header(FILE *fp, const CompressHeader *header);
BackupResult read_compress_header(FILE *fp, CompressHeader *header);

// Huffman压缩相关函数
BackupResult huffman_compress(FILE *input_fp, FILE *output_fp);
BackupResult huffman_decompress(FILE *input_fp, FILE *output_fp, unsigned long original_size);

// LZ77压缩相关函数
BackupResult lz77_compress(FILE *input_fp, FILE *output_fp);
BackupResult lz77_decompress(FILE *input_fp, FILE *output_fp, unsigned long original_size);

// 对外API函数
BackupResult compress_file(const char *input_path, const char *output_path, CompressAlgorithm algorithm);
BackupResult decompress_file(const char *input_path, const char *output_path, CompressAlgorithm algorithm);

#endif // COMPRESS_H
