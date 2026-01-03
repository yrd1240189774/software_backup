#include "../include/main.h"
#include <stdio.h>
#include <windows.h>
#include <time.h>

// 打印帮助信息
void print_help() {
    printf("数据备份软件使用说明：\n");
    printf("\n");
    printf("备份功能：\n");
    printf("  backup -s <源路径> -t <目标路径> [选项]\n");
    printf("  选项：\n");
    printf("    -a <算法>：打包算法（tar/zip）\n");
    printf("    -c <算法>：压缩算法（none/haff/lz77）\n");
    printf("    -e <算法> <密钥>：加密算法（none/aes/des）和密钥\n");
    printf("\n");
    printf("定时备份功能：\n");
    printf("  schedule -s <源路径> -t <目标路径> -i <间隔(分钟)> [选项]\n");
    printf("  选项：\n");
    printf("    -i <间隔>：备份间隔（分钟）\n");
    printf("    -a <算法>：打包算法（tar/zip）\n");
    printf("    -c <算法>：压缩算法（none/haff/lz77）\n");
    printf("    -e <算法> <密钥>：加密算法（none/aes/des）和密钥\n");
    printf("    --keep-days <天数>：保留备份的天数\n");
    printf("    --max-count <数量>：最大备份文件数量\n");
    printf("\n");
    printf("数据清理功能：\n");
    printf("  cleanup -d <备份目录> --keep-days <天数> [选项]\n");
    printf("  选项：\n");
    printf("    -d <目录>：备份文件所在目录\n");
    printf("    --keep-days <天数>：保留备份的天数\n");
    printf("    --max-count <数量>：最大备份文件数量\n");
    printf("\n");
    printf("还原功能：\n");
    printf("  restore -f <备份文件> -t <目标路径> [选项]\n");
    printf("  选项：\n");
    printf("    -e <算法> <密钥>：解密算法（none/aes/des）和密钥\n");
    printf("\n");
    printf("压缩功能：\n");
    printf("  compress -i <输入文件> -o <输出文件> -a <算法>\n");
    printf("  选项：\n");
    printf("    -a <算法>：压缩算法（haff/lz77）\n");
    printf("\n");
    printf("解压功能：\n");
    printf("  decompress -i <输入文件> -o <输出文件>\n");
    printf("\n");
    printf("加密功能：\n");
    printf("  encrypt -i <输入文件> -o <输出文件> -a <算法> -k <密钥>\n");
    printf("  选项：\n");
    printf("    -a <算法>：加密算法（aes/des）\n");
    printf("    -k <密钥>：加密密钥\n");
    printf("\n");
    printf("解密功能：\n");
    printf("  decrypt -i <输入文件> -o <输出文件> -a <算法> -k <密钥>\n");
    printf("  选项：\n");
    printf("    -a <算法>：解密算法（aes/des）\n");
    printf("    -k <密钥>：解密密钥\n");
    printf("\n");
    printf("示例：\n");
    printf("  backup -s C:\\data -t D:\\backup -a tar -c haff -e aes 123456\n");
    printf("  schedule -s C:\\data -t D:\\backup -i 60 -c haff -e aes 123456 --keep-days 7 --max-count 5\n");
    printf("  cleanup -d D:\\backup --keep-days 7 --max-count 5\n");
    printf("  restore -f D:\\backup\\backup.dat -t C:\\restore -e aes 123456\n");
    printf("  compress -i input.txt -o output.cmp -a haff\n");
    printf("  decompress -i input.cmp -o output.txt\n");
    printf("  encrypt -i input.txt -o output.enc -a aes -k 123456\n");
    printf("  decrypt -i input.enc -o output.txt -a aes -k 123456\n");
}

// 解析命令行参数
int parse_args(int argc, char *argv[], int *operation, BackupOptions *backup_opt, RestoreOptions *restore_opt,
               char *compress_input, char *compress_output, CompressAlgorithm *compress_algorithm,
               char *decompress_input, char *decompress_output,
               char *encrypt_input, char *encrypt_output, EncryptAlgorithm *encrypt_algorithm, char *encrypt_key,
               char *decrypt_input, char *decrypt_output, EncryptAlgorithm *decrypt_algorithm, char *decrypt_key,
               ScheduledBackupOptions *scheduled_backup_opt, char *cleanup_dir, int *keep_days, int *max_count) {
    if (argc < 2) {
        return -1;
    }

    // 确定操作类型
    if (strcmp(argv[1], "backup") == 0) {
        *operation = 0; // 备份
    } else if (strcmp(argv[1], "schedule") == 0) {
        *operation = 6; // 定时备份
    } else if (strcmp(argv[1], "cleanup") == 0) {
        *operation = 7; // 数据清理
    } else if (strcmp(argv[1], "restore") == 0) {
        *operation = 1; // 还原
    } else if (strcmp(argv[1], "compress") == 0) {
        *operation = 2; // 压缩
    } else if (strcmp(argv[1], "decompress") == 0) {
        *operation = 3; // 解压
    } else if (strcmp(argv[1], "encrypt") == 0) {
        *operation = 4; // 加密
    } else if (strcmp(argv[1], "decrypt") == 0) {
        *operation = 5; // 解密
    } else {
        return -1;
    }

    // 解析备份参数
    if (*operation == 0) {
        int i = 2;
        while (i < argc) {
            if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
                strcpy(backup_opt->source_path, argv[i + 1]);
                i += 2;
            } else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
                strcpy(backup_opt->target_path, argv[i + 1]);
                i += 2;
            } else if (strcmp(argv[i], "-a") == 0 && i + 1 < argc) {
                if (strcmp(argv[i + 1], "tar") == 0) {
                    backup_opt->pack_algorithm = PACK_ALGORITHM_TAR;
                } else if (strcmp(argv[i + 1], "zip") == 0) {
                    backup_opt->pack_algorithm = PACK_ALGORITHM_ZIP;
                }
                i += 2;
            } else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
                if (strcmp(argv[i + 1], "none") == 0) {
                    backup_opt->compress_algorithm = COMPRESS_ALGORITHM_NONE;
                } else if (strcmp(argv[i + 1], "haff") == 0) {
                    backup_opt->compress_algorithm = COMPRESS_ALGORITHM_HAFF;
                } else if (strcmp(argv[i + 1], "lz77") == 0) {
                    backup_opt->compress_algorithm = COMPRESS_ALGORITHM_LZ77;
                }
                i += 2;
            } else if (strcmp(argv[i], "-e") == 0 && i + 2 < argc) {
                backup_opt->encrypt_enable = 1;
                if (strcmp(argv[i + 1], "none") == 0) {
                    backup_opt->encrypt_algorithm = ENCRYPT_ALGORITHM_NONE;
                } else if (strcmp(argv[i + 1], "aes") == 0) {
                    backup_opt->encrypt_algorithm = ENCRYPT_ALGORITHM_AES;
                } else if (strcmp(argv[i + 1], "des") == 0) {
                    backup_opt->encrypt_algorithm = ENCRYPT_ALGORITHM_DES;
                }
                strcpy(backup_opt->encrypt_key, argv[i + 2]);
                i += 3;
            } else {
                return -1;
            }
        }

        // 检查必需参数
        if (backup_opt->source_path[0] == 0 || backup_opt->target_path[0] == 0) {
            return -1;
        }
    }
    // 解析定时备份参数
    else if (*operation == 6) {
        int i = 2;
        while (i < argc) {
            if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
                strcpy(scheduled_backup_opt->backup_options.source_path, argv[i + 1]);
                i += 2;
            } else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
                strcpy(scheduled_backup_opt->backup_options.target_path, argv[i + 1]);
                i += 2;
            } else if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
                scheduled_backup_opt->schedule_config.interval_minutes = atoi(argv[i + 1]);
                scheduled_backup_opt->schedule_config.enable = 1;
                i += 2;
            } else if (strcmp(argv[i], "-a") == 0 && i + 1 < argc) {
                if (strcmp(argv[i + 1], "tar") == 0) {
                    scheduled_backup_opt->backup_options.pack_algorithm = PACK_ALGORITHM_TAR;
                } else if (strcmp(argv[i + 1], "zip") == 0) {
                    scheduled_backup_opt->backup_options.pack_algorithm = PACK_ALGORITHM_ZIP;
                }
                i += 2;
            } else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
                if (strcmp(argv[i + 1], "none") == 0) {
                    scheduled_backup_opt->backup_options.compress_algorithm = COMPRESS_ALGORITHM_NONE;
                } else if (strcmp(argv[i + 1], "haff") == 0) {
                    scheduled_backup_opt->backup_options.compress_algorithm = COMPRESS_ALGORITHM_HAFF;
                } else if (strcmp(argv[i + 1], "lz77") == 0) {
                    scheduled_backup_opt->backup_options.compress_algorithm = COMPRESS_ALGORITHM_LZ77;
                }
                i += 2;
            } else if (strcmp(argv[i], "-e") == 0 && i + 2 < argc) {
                scheduled_backup_opt->backup_options.encrypt_enable = 1;
                if (strcmp(argv[i + 1], "none") == 0) {
                    scheduled_backup_opt->backup_options.encrypt_algorithm = ENCRYPT_ALGORITHM_NONE;
                } else if (strcmp(argv[i + 1], "aes") == 0) {
                    scheduled_backup_opt->backup_options.encrypt_algorithm = ENCRYPT_ALGORITHM_AES;
                } else if (strcmp(argv[i + 1], "des") == 0) {
                    scheduled_backup_opt->backup_options.encrypt_algorithm = ENCRYPT_ALGORITHM_DES;
                }
                strcpy(scheduled_backup_opt->backup_options.encrypt_key, argv[i + 2]);
                i += 3;
            } else if (strcmp(argv[i], "--keep-days") == 0 && i + 1 < argc) {
                scheduled_backup_opt->cleanup_config.keep_days = atoi(argv[i + 1]);
                scheduled_backup_opt->cleanup_config.enable = 1;
                i += 2;
            } else if (strcmp(argv[i], "--max-count") == 0 && i + 1 < argc) {
                scheduled_backup_opt->cleanup_config.max_backup_count = atoi(argv[i + 1]);
                scheduled_backup_opt->cleanup_config.enable = 1;
                i += 2;
            } else {
                return -1;
            }
        }

        // 检查必需参数
        if (scheduled_backup_opt->backup_options.source_path[0] == 0 || 
            scheduled_backup_opt->backup_options.target_path[0] == 0 || 
            scheduled_backup_opt->schedule_config.interval_minutes <= 0) {
            return -1;
        }
        
        // 设置清理配置的备份目录
        strcpy(scheduled_backup_opt->cleanup_config.backup_directory, scheduled_backup_opt->backup_options.target_path);
    }
    // 解析数据清理参数
    else if (*operation == 7) {
        int i = 2;
        while (i < argc) {
            if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
                strcpy(cleanup_dir, argv[i + 1]);
                i += 2;
            } else if (strcmp(argv[i], "--keep-days") == 0 && i + 1 < argc) {
                *keep_days = atoi(argv[i + 1]);
                i += 2;
            } else if (strcmp(argv[i], "--max-count") == 0 && i + 1 < argc) {
                *max_count = atoi(argv[i + 1]);
                i += 2;
            } else {
                return -1;
            }
        }

        // 检查必需参数
        if (cleanup_dir[0] == 0 || (*keep_days <= 0 && *max_count <= 0)) {
            return -1;
        }
    }
    // 解析还原参数
    else if (*operation == 1) {
        int i = 2;
        while (i < argc) {
            if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
                strcpy(restore_opt->backup_file, argv[i + 1]);
                i += 2;
            } else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
                strcpy(restore_opt->target_path, argv[i + 1]);
                i += 2;
            } else if (strcmp(argv[i], "-e") == 0 && i + 2 < argc) {
                restore_opt->encrypt_enable = 1;
                if (strcmp(argv[i + 1], "none") == 0) {
                    restore_opt->encrypt_algorithm = ENCRYPT_ALGORITHM_NONE;
                } else if (strcmp(argv[i + 1], "aes") == 0) {
                    restore_opt->encrypt_algorithm = ENCRYPT_ALGORITHM_AES;
                } else if (strcmp(argv[i + 1], "des") == 0) {
                    restore_opt->encrypt_algorithm = ENCRYPT_ALGORITHM_DES;
                }
                strcpy(restore_opt->encrypt_key, argv[i + 2]);
                i += 3;
            } else {
                return -1;
            }
        }

        // 检查必需参数
        if (restore_opt->backup_file[0] == 0 || restore_opt->target_path[0] == 0) {
            return -1;
        }
    }
    // 解析压缩参数
    else if (*operation == 2) {
        int i = 2;
        while (i < argc) {
            if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
                strcpy(compress_input, argv[i + 1]);
                i += 2;
            } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
                strcpy(compress_output, argv[i + 1]);
                i += 2;
            } else if (strcmp(argv[i], "-a") == 0 && i + 1 < argc) {
                if (strcmp(argv[i + 1], "haff") == 0) {
                    *compress_algorithm = COMPRESS_ALGORITHM_HAFF;
                } else if (strcmp(argv[i + 1], "lz77") == 0) {
                    *compress_algorithm = COMPRESS_ALGORITHM_LZ77;
                } else {
                    return -1;
                }
                i += 2;
            } else {
                return -1;
            }
        }

        // 检查必需参数
        if (compress_input[0] == 0 || compress_output[0] == 0 || *compress_algorithm == COMPRESS_ALGORITHM_NONE) {
            return -1;
        }
    }
    // 解析解压参数
    else if (*operation == 3) {
        int i = 2;
        while (i < argc) {
            if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
                strcpy(decompress_input, argv[i + 1]);
                i += 2;
            } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
                strcpy(decompress_output, argv[i + 1]);
                i += 2;
            } else {
                return -1;
            }
        }

        // 检查必需参数
        if (decompress_input[0] == 0 || decompress_output[0] == 0) {
            return -1;
        }
    }
    // 解析加密参数
    else if (*operation == 4) {
        int i = 2;
        while (i < argc) {
            if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
                strcpy(encrypt_input, argv[i + 1]);
                i += 2;
            } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
                strcpy(encrypt_output, argv[i + 1]);
                i += 2;
            } else if (strcmp(argv[i], "-a") == 0 && i + 1 < argc) {
                if (strcmp(argv[i + 1], "aes") == 0) {
                    *encrypt_algorithm = ENCRYPT_ALGORITHM_AES;
                } else if (strcmp(argv[i + 1], "des") == 0) {
                    *encrypt_algorithm = ENCRYPT_ALGORITHM_DES;
                } else {
                    return -1;
                }
                i += 2;
            } else if (strcmp(argv[i], "-k") == 0 && i + 1 < argc) {
                strcpy(encrypt_key, argv[i + 1]);
                i += 2;
            } else {
                return -1;
            }
        }

        // 检查必需参数
        if (encrypt_input[0] == 0 || encrypt_output[0] == 0 || *encrypt_algorithm == ENCRYPT_ALGORITHM_NONE || encrypt_key[0] == 0) {
            return -1;
        }
    }
    // 解析解密参数
    else if (*operation == 5) {
        int i = 2;
        while (i < argc) {
            if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
                strcpy(decrypt_input, argv[i + 1]);
                i += 2;
            } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
                strcpy(decrypt_output, argv[i + 1]);
                i += 2;
            } else if (strcmp(argv[i], "-a") == 0 && i + 1 < argc) {
                if (strcmp(argv[i + 1], "aes") == 0) {
                    *decrypt_algorithm = ENCRYPT_ALGORITHM_AES;
                } else if (strcmp(argv[i + 1], "des") == 0) {
                    *decrypt_algorithm = ENCRYPT_ALGORITHM_DES;
                } else {
                    return -1;
                }
                i += 2;
            } else if (strcmp(argv[i], "-k") == 0 && i + 1 < argc) {
                strcpy(decrypt_key, argv[i + 1]);
                i += 2;
            } else {
                return -1;
            }
        }

        // 检查必需参数
        if (decrypt_input[0] == 0 || decrypt_output[0] == 0 || *decrypt_algorithm == ENCRYPT_ALGORITHM_NONE || decrypt_key[0] == 0) {
            return -1;
        }
    }

    return 0;
}

int main(int argc, char *argv[]) {
    // 设置控制台代码页为UTF-8，解决中文乱码问题
    SetConsoleOutputCP(CP_UTF8);
    
    int operation = -1; // 0: 备份, 1: 还原, 2: 压缩, 3: 解压, 4: 加密, 5: 解密, 6: 定时备份, 7: 数据清理
    BackupOptions backup_opt = {0};
    RestoreOptions restore_opt = {0};
    ScheduledBackupOptions scheduled_backup_opt = {0};
    BackupResult result;
    
    // 压缩相关参数
    char compress_input[256] = {0};
    char compress_output[256] = {0};
    CompressAlgorithm compress_algorithm = COMPRESS_ALGORITHM_NONE;
    
    // 解压相关参数
    char decompress_input[256] = {0};
    char decompress_output[256] = {0};
    
    // 加密相关参数
    char encrypt_input[256] = {0};
    char encrypt_output[256] = {0};
    EncryptAlgorithm encrypt_algorithm = ENCRYPT_ALGORITHM_NONE;
    char encrypt_key[64] = {0};
    
    // 解密相关参数
    char decrypt_input[256] = {0};
    char decrypt_output[256] = {0};
    EncryptAlgorithm decrypt_algorithm = ENCRYPT_ALGORITHM_NONE;
    char decrypt_key[64] = {0};
    
    // 定时备份和数据清理相关参数
    ScheduledBackupOptions scheduled_backup_opt_local = {0};
    char cleanup_dir[256] = {0};
    int keep_days = 0;
    int max_count = 0;

    // 设置默认值
    backup_opt.file_types = FILE_TYPE_REGULAR | FILE_TYPE_DIRECTORY;
    backup_opt.pack_algorithm = PACK_ALGORITHM_TAR;
    backup_opt.compress_algorithm = COMPRESS_ALGORITHM_NONE;
    backup_opt.encrypt_enable = 0;
    backup_opt.encrypt_algorithm = ENCRYPT_ALGORITHM_NONE;
    
    // 初始化大小范围：允许所有大小的文件
    backup_opt.size_range.min_size = 0;
    backup_opt.size_range.max_size = 0xFFFFFFFF;
    
    restore_opt.encrypt_enable = 0;
    restore_opt.encrypt_algorithm = ENCRYPT_ALGORITHM_NONE;
    
    // 初始化定时备份配置
    scheduled_backup_opt_local.schedule_config.enable = 0;
    scheduled_backup_opt_local.schedule_config.interval_minutes = 0;
    scheduled_backup_opt_local.cleanup_config.enable = 0;
    scheduled_backup_opt_local.cleanup_config.keep_days = 7;  // 默认保留7天
    scheduled_backup_opt_local.cleanup_config.max_backup_count = 5;  // 默认最多保留5个备份
    
    // 设置定时备份的备份选项默认值
    scheduled_backup_opt_local.backup_options.file_types = FILE_TYPE_REGULAR | FILE_TYPE_DIRECTORY;
    scheduled_backup_opt_local.backup_options.pack_algorithm = PACK_ALGORITHM_TAR;
    scheduled_backup_opt_local.backup_options.compress_algorithm = COMPRESS_ALGORITHM_NONE;
    scheduled_backup_opt_local.backup_options.encrypt_enable = 0;
    scheduled_backup_opt_local.backup_options.encrypt_algorithm = ENCRYPT_ALGORITHM_NONE;
    
    // 初始化大小范围：允许所有大小的文件
    scheduled_backup_opt_local.backup_options.size_range.min_size = 0;
    scheduled_backup_opt_local.backup_options.size_range.max_size = 0xFFFFFFFF;
    
    scheduled_backup_opt_local.backup_options.create_time_range.enable = 0;
    scheduled_backup_opt_local.backup_options.modify_time_range.enable = 0;
    scheduled_backup_opt_local.backup_options.access_time_range.enable = 0;
    
    // 初始化排除选项
    scheduled_backup_opt_local.backup_options.exclude_usergroup.enable_user = 0;
    scheduled_backup_opt_local.backup_options.exclude_usergroup.enable_group = 0;
    scheduled_backup_opt_local.backup_options.exclude_usergroup.user_count = 0;
    scheduled_backup_opt_local.backup_options.exclude_usergroup.group_count = 0;
    scheduled_backup_opt_local.backup_options.exclude_filename.pattern[0] = '\0';
    scheduled_backup_opt_local.backup_options.exclude_directory.count = 0;

    // 解析命令行参数
    if (parse_args(argc, argv, &operation, &backup_opt, &restore_opt,
                   compress_input, compress_output, &compress_algorithm,
                   decompress_input, decompress_output,
                   encrypt_input, encrypt_output, &encrypt_algorithm, encrypt_key,
                   decrypt_input, decrypt_output, &decrypt_algorithm, decrypt_key,
                   &scheduled_backup_opt_local, cleanup_dir, &keep_days, &max_count) != 0) {
        print_help();
        return 1;
    }

    // 执行操作
    switch (operation) {
        case 0: // 备份
            printf("开始备份数据...\n");
            printf("源路径: %s\n", backup_opt.source_path);
            printf("目标路径: %s\n", backup_opt.target_path);
            
            result = backup_data(&backup_opt);
            if (result == BACKUP_SUCCESS) {
                printf("备份成功！\n");
            } else {
                printf("备份失败，错误码: %d\n", result);
            }
            break;
        
        case 1: // 还原
            printf("开始还原数据...\n");
            printf("备份文件: %s\n", restore_opt.backup_file);
            printf("目标路径: %s\n", restore_opt.target_path);
            
            result = restore_data(&restore_opt);
            if (result == BACKUP_SUCCESS) {
                printf("还原成功！\n");
            } else {
                printf("还原失败，错误码: %d\n", result);
            }
            break;
            
        case 2: // 压缩
            printf("开始压缩文件...\n");
            printf("输入文件: %s\n", compress_input);
            printf("输出文件: %s\n", compress_output);
            
            result = compress_file(compress_input, compress_output, compress_algorithm);
            if (result == BACKUP_SUCCESS) {
                printf("压缩成功！\n");
            } else {
                printf("压缩失败，错误码: %d\n", result);
            }
            break;
            
        case 3: // 解压
            printf("开始解压文件...\n");
            printf("输入文件: %s\n", decompress_input);
            printf("输出文件: %s\n", decompress_output);
            
            result = decompress_file(decompress_input, decompress_output, COMPRESS_ALGORITHM_NONE);
            if (result == BACKUP_SUCCESS) {
                printf("解压成功！\n");
            } else {
                printf("解压失败，错误码: %d\n", result);
            }
            break;
            
        case 4: // 加密
            printf("开始加密文件...\n");
            printf("输入文件: %s\n", encrypt_input);
            printf("输出文件: %s\n", encrypt_output);
            
            result = encrypt_file(encrypt_input, encrypt_output, encrypt_key);
            if (result == BACKUP_SUCCESS) {
                printf("加密成功！\n");
            } else {
                printf("加密失败，错误码: %d\n", result);
            }
            break;
            
        case 5: // 解密
            printf("开始解密文件...\n");
            printf("输入文件: %s\n", decrypt_input);
            printf("输出文件: %s\n", decrypt_output);
            
            result = decrypt_file(decrypt_input, decrypt_output, decrypt_key);
            if (result == BACKUP_SUCCESS) {
                printf("解密成功！\n");
            } else {
                printf("解密失败，错误码: %d\n", result);
            }
            break;
            
        case 6: // 定时备份
            printf("开始定时备份...\n");
            printf("源路径: %s\n", scheduled_backup_opt_local.backup_options.source_path);
            printf("目标路径: %s\n", scheduled_backup_opt_local.backup_options.target_path);
            printf("备份间隔: %d 分钟\n", scheduled_backup_opt_local.schedule_config.interval_minutes);
            
            if (scheduled_backup_opt_local.cleanup_config.enable) {
                printf("数据清理配置:\n");
                printf("  保留天数: %d\n", scheduled_backup_opt_local.cleanup_config.keep_days);
                printf("  最大备份数: %d\n", scheduled_backup_opt_local.cleanup_config.max_backup_count);
            }
            
            result = schedule_backup(&scheduled_backup_opt_local);
            if (result == BACKUP_SUCCESS) {
                printf("定时备份配置成功！\n");
            } else {
                printf("定时备份配置失败，错误码: %d\n", result);
            }
            break;
            
        case 7: // 数据清理
            printf("开始清理旧备份...\n");
            printf("备份目录: %s\n", cleanup_dir);
            if (keep_days > 0) printf("保留天数: %d\n", keep_days);
            if (max_count > 0) printf("最大备份数: %d\n", max_count);
            
            // 创建临时的清理配置
            CleanupConfig cleanup_config = {0};
            cleanup_config.enable = 1;
            strcpy(cleanup_config.backup_directory, cleanup_dir);
            cleanup_config.keep_days = keep_days;
            cleanup_config.max_backup_count = max_count;
            
            result = cleanup_old_backups(&cleanup_config);
            if (result == BACKUP_SUCCESS) {
                printf("数据清理完成！\n");
            } else {
                printf("数据清理失败，错误码: %d\n", result);
            }
            break;
    }

    return 0;
}