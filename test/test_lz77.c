#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"

// 简单的测试程序，用于验证LZ77压缩解压功能
int main() {
    char input[] = "ABABABABABABABAB";
    char input_file[] = "test_input.txt";
    char compressed_file[] = "test_compressed.bin";
    char decompressed_file[] = "test_decompressed.txt";
    FILE *fp;
    
    // 创建测试输入文件
    fp = fopen(input_file, "wb");
    if (fp == NULL) {
        printf("Failed to create test input file\n");
        return 1;
    }
    fwrite(input, 1, strlen(input), fp);
    fclose(fp);
    
    printf("Original input: %s\n", input);
    
    // 执行压缩
    printf("\n=== Compression ===\n");
    BackupResult result = compress_file(input_file, compressed_file, COMPRESS_ALGORITHM_LZ77);
    if (result != BACKUP_SUCCESS) {
        printf("Compression failed\n");
        return 1;
    }
    
    // 执行解压
    printf("\n=== Decompression ===\n");
    result = decompress_file(compressed_file, decompressed_file, COMPRESS_ALGORITHM_LZ77);
    if (result != BACKUP_SUCCESS) {
        printf("Decompression failed\n");
        return 1;
    }
    
    // 读取并输出解压结果
    fp = fopen(decompressed_file, "rb");
    if (fp == NULL) {
        printf("Failed to open decompressed file for reading\n");
        return 1;
    }
    
    printf("\n=== Decompression Result ===\n");
    char c;
    while ((c = fgetc(fp)) != EOF) {
        putchar(c);
    }
    printf("\n");
    
    fclose(fp);
    
    printf("\n=== Test Complete ===\n");
    
    // 比较原始输入和解压结果
    fp = fopen(decompressed_file, "rb");
    char *decompressed = (char *)malloc(strlen(input) + 1);
    if (decompressed == NULL) {
        printf("Failed to allocate memory\n");
        fclose(fp);
        return 1;
    }
    
    size_t bytes_read = fread(decompressed, 1, strlen(input), fp);
    decompressed[bytes_read] = '\0';
    fclose(fp);
    
    if (strcmp(input, decompressed) == 0) {
        printf("\n✅ SUCCESS: Decompressed output matches original input!\n");
    } else {
        printf("\n❌ FAILURE: Decompressed output does not match original input!\n");
        printf("Expected: %s\n", input);
        printf("Got: %s\n", decompressed);
    }
    
    free(decompressed);
    
    return 0;
}