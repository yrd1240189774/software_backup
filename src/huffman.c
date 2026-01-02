#include "../include/compress.h"
#include <stdlib.h>
#include <string.h>

// 计算字符频率
void calculate_frequency(FILE *input_fp, unsigned long frequency[256]) {
    unsigned char buffer[4096];
    size_t bytes_read;
    
    // 初始化频率数组
    memset(frequency, 0, 256 * sizeof(unsigned long));
    
    // 读取文件并计算频率
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), input_fp)) > 0) {
        for (size_t i = 0; i < bytes_read; i++) {
            frequency[buffer[i]]++;
        }
    }
    
    // 重置文件指针到文件开头
    fseek(input_fp, 0, SEEK_SET);
}

// 创建Huffman树节点
HuffmanNode* create_huffman_node(unsigned char data, unsigned long frequency) {
    HuffmanNode *node = (HuffmanNode *)malloc(sizeof(HuffmanNode));
    if (node == NULL) {
        return NULL;
    }
    
    node->data = data;
    node->frequency = frequency;
    node->left = NULL;
    node->right = NULL;
    
    return node;
}

// 交换两个Huffman树节点
void swap_huffman_nodes(HuffmanNode **a, HuffmanNode **b) {
    HuffmanNode *temp = *a;
    *a = *b;
    *b = temp;
}

// 堆化函数，用于构建最小堆
void heapify(HuffmanNode **heap, int size, int index) {
    int smallest = index;
    int left = 2 * index + 1;
    int right = 2 * index + 2;
    
    if (left < size && heap[left]->frequency < heap[smallest]->frequency) {
        smallest = left;
    }
    
    if (right < size && heap[right]->frequency < heap[smallest]->frequency) {
        smallest = right;
    }
    
    if (smallest != index) {
        swap_huffman_nodes(&heap[index], &heap[smallest]);
        heapify(heap, size, smallest);
    }
}

// 构建最小堆
void build_min_heap(HuffmanNode **heap, int size) {
    for (int i = size / 2 - 1; i >= 0; i--) {
        heapify(heap, size, i);
    }
}

// 提取最小节点
HuffmanNode* extract_min_node(HuffmanNode **heap, int *size) {
    HuffmanNode *min_node = heap[0];
    heap[0] = heap[*size - 1];
    (*size)--;
    heapify(heap, *size, 0);
    
    return min_node;
}

// 插入节点到最小堆
void insert_heap(HuffmanNode **heap, int *size, HuffmanNode *node) {
    (*size)++;
    int i = *size - 1;
    
    heap[i] = node;
    
    while (i != 0 && heap[(i - 1) / 2]->frequency > heap[i]->frequency) {
        swap_huffman_nodes(&heap[i], &heap[(i - 1) / 2]);
        i = (i - 1) / 2;
    }
}

// 检查是否是叶子节点
int is_leaf_node(HuffmanNode *node) {
    return (node->left == NULL && node->right == NULL);
}

// 构建Huffman树
HuffmanNode* build_huffman_tree(unsigned long frequency[256]) {
    HuffmanNode *left, *right, *top;
    int size = 0;
    
    // 创建堆，只包含频率大于0的字符
    HuffmanNode **heap = (HuffmanNode **)malloc(256 * sizeof(HuffmanNode *));
    if (heap == NULL) {
        return NULL;
    }
    
    for (int i = 0; i < 256; i++) {
        if (frequency[i] > 0) {
            heap[size] = create_huffman_node((unsigned char)i, frequency[i]);
            if (heap[size] == NULL) {
                free(heap);
                return NULL;
            }
            size++;
        }
    }
    
    // 如果只有一个字符，特殊处理
    if (size == 1) {
        HuffmanNode *node = create_huffman_node(heap[0]->data, heap[0]->frequency);
        free(heap[0]);
        free(heap);
        return node;
    }
    
    // 构建最小堆
    build_min_heap(heap, size);
    
    // 构建Huffman树
    while (size > 1) {
        // 提取两个最小频率的节点
        left = extract_min_node(heap, &size);
        right = extract_min_node(heap, &size);
        
        // 创建一个新的内部节点，频率为两个节点频率之和
        top = create_huffman_node(0, left->frequency + right->frequency);
        if (top == NULL) {
            free(heap);
            return NULL;
        }
        top->left = left;
        top->right = right;
        
        // 将新节点插入堆中
        insert_heap(heap, &size, top);
    }
    
    // 提取根节点
    HuffmanNode *root = extract_min_node(heap, &size);
    free(heap);
    
    return root;
}

// 生成Huffman编码
void generate_huffman_codes(HuffmanNode *root, unsigned char code[], int top, char codes[256][256]) {
    // 左子树，编码为0
    if (root->left != NULL) {
        code[top] = '0';
        generate_huffman_codes(root->left, code, top + 1, codes);
    }
    
    // 右子树，编码为1
    if (root->right != NULL) {
        code[top] = '1';
        generate_huffman_codes(root->right, code, top + 1, codes);
    }
    
    // 叶子节点，保存编码
    if (is_leaf_node(root)) {
        code[top] = '\0';
        strcpy(codes[root->data], (char*)code);
    }
}

// Huffman压缩实现
BackupResult huffman_compress(FILE *input_fp, FILE *output_fp) {
    unsigned long frequency[256];
    HuffmanNode *root;
    unsigned char code[256];
    char codes[256][256];
    CompressHeader header;
    
    // 计算字符频率
    calculate_frequency(input_fp, frequency);
    
    // 构建Huffman树
    root = build_huffman_tree(frequency);
    if (root == NULL) {
        return BACKUP_ERROR_MEMORY;
    }
    
    // 生成Huffman编码
    generate_huffman_codes(root, code, 0, codes);
    
    // 写入压缩文件头部
    strcpy((char*)header.magic, "COMP");
    header.version = 1;
    header.algorithm = COMPRESS_ALGORITHM_HAFF;
    fseek(input_fp, 0, SEEK_END);
    header.original_size = ftell(input_fp);
    fseek(input_fp, 0, SEEK_SET);
    header.compressed_size = 0; // 后续更新
    
    fwrite(&header, sizeof(CompressHeader), 1, output_fp);
    
    // 写入频率表，用于解压
    fwrite(frequency, sizeof(unsigned long), 256, output_fp);
    
    // 压缩文件数据
    unsigned char buffer[4096];
    unsigned char output_buffer = 0;
    int bit_count = 0;
    size_t bytes_read;
    
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), input_fp)) > 0) {
        for (size_t i = 0; i < bytes_read; i++) {
            char *huff_code = codes[buffer[i]];
            for (int j = 0; huff_code[j] != '\0'; j++) {
                // 将Huffman编码写入输出缓冲区
                output_buffer <<= 1;
                if (huff_code[j] == '1') {
                    output_buffer |= 1;
                }
                bit_count++;
                
                // 如果输出缓冲区已满，写入文件
                if (bit_count == 8) {
                    fwrite(&output_buffer, 1, 1, output_fp);
                    output_buffer = 0;
                    bit_count = 0;
                }
            }
        }
    }
    
    // 处理剩余的位
    if (bit_count > 0) {
        output_buffer <<= (8 - bit_count);
        fwrite(&output_buffer, 1, 1, output_fp);
    }
    
    // 更新压缩后文件大小
    fseek(output_fp, 0, SEEK_END);
    header.compressed_size = ftell(output_fp) - sizeof(CompressHeader);
    fseek(output_fp, 0, SEEK_SET);
    fwrite(&header, sizeof(CompressHeader), 1, output_fp);
    
    // 释放Huffman树
    // 注意：这里应该实现一个释放Huffman树的函数，暂时省略
    
    return BACKUP_SUCCESS;
}

// Huffman解压实现
BackupResult huffman_decompress(FILE *input_fp, FILE *output_fp) {
    CompressHeader header;
    unsigned long frequency[256];
    HuffmanNode *root;
    unsigned char buffer[4096];
    size_t bytes_read;
    unsigned long bytes_written = 0;
    
    // 读取压缩文件头部
    fread(&header, sizeof(CompressHeader), 1, input_fp);
    
    // 读取频率表
    fread(frequency, sizeof(unsigned long), 256, input_fp);
    
    // 重建Huffman树
    root = build_huffman_tree(frequency);
    if (root == NULL) {
        return BACKUP_ERROR_MEMORY;
    }
    
    // 解压文件数据
    HuffmanNode *current_node = root;
    unsigned char input_byte;
    
    while ((bytes_read = fread(&input_byte, 1, 1, input_fp)) > 0) {
        for (int i = 7; i >= 0; i--) {
            unsigned char bit = (input_byte >> i) & 1;
            
            if (bit == 0) {
                current_node = current_node->left;
            } else {
                current_node = current_node->right;
            }
            
            if (is_leaf_node(current_node)) {
                fwrite(&current_node->data, 1, 1, output_fp);
                bytes_written++;
                current_node = root;
                
                // 检查是否已经解压完所有原始数据
                if (bytes_written == header.original_size) {
                    break;
                }
            }
        }
        
        if (bytes_written == header.original_size) {
            break;
        }
    }
    
    // 释放Huffman树
    // 注意：这里应该实现一个释放Huffman树的函数，暂时省略
    
    return BACKUP_SUCCESS;
}
