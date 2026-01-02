# 编译器
CC = gcc

# 编译选项
CFLAGS = -Wall -Iinclude -g 

# OpenSSL路径配置（根据你的实际安装路径修改）
OPENSSL_DIR = C:/Program Files/OpenSSL-Win64
OPENSSL_INCLUDE = $(OPENSSL_DIR)/include
OPENSSL_LIB = $(OPENSSL_DIR)/lib

CFLAGS += -I"$(OPENSSL_INCLUDE)"
LDFLAGS = -L"$(OPENSSL_LIB)" -lssl -lcrypto -lws2_32 -lcrypt32

# 目标文件
TARGET = backup_software

# 源文件
SRCS = src/main.c src/backup.c src/restore.c src/filter.c src/pack.c src/compress.c src/encrypt.c src/metadata.c src/huffman.c src/traverse.c

# 目标文件 - 输出到build目录
OBJS = $(patsubst src/%.c,build/%.o,$(SRCS))

# 默认目标
all: $(TARGET)

# 编译目标
$(TARGET): build $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

# 创建build目录
build:
	@mkdir build 2>nul || echo build directory exists

# 编译源文件 - 输出到build目录
build/%.o: src/%.c
	@mkdir build 2>nul || echo build directory exists
	$(CC) $(CFLAGS) -c $< -o $@

# 清理目标
clean:
	@del /f /q build\*.o 2>nul || echo nothing to clean
	@rmdir /s /q build 2>nul || echo build directory removed
	@del /f /q $(TARGET).exe 2>nul || echo executable removed

# 基础功能测试
test-basic: $(TARGET)
	@echo Running basic functionality test...
	@$(TARGET) --help > nul 2>&1 || exit 0 

# 压缩功能测试
compress-test: $(TARGET)
	@echo Creating test directory...
	@mkdir test 2>nul || echo test directory exists
	@mkdir build 2>nul || echo build directory exists
	@echo "This is a test file for compression. It contains some repetitive content to test the compression algorithm. This is a test file for compression. It contains some repetitive content to test the compression algorithm." > test\compress_test.txt
	@echo Running compression test with LZ77 algorithm...
	@$(TARGET) compress -i test\compress_test.txt -o build\compressed_lz77.cmp -a lz77
	@echo Running decompression test with LZ77 algorithm...
	@$(TARGET) decompress -i build\compressed_lz77.cmp -o build\decompressed_lz77.txt
	@echo Verifying LZ77 compression/decompression...
	@fc test\compress_test.txt build\decompressed_lz77.txt && echo  LZ77 COMPRESS/DECOMPRESS: FILE MATCH || echo  LZ77 COMPRESS/DECOMPRESS: FILE DIFFER
	@echo Running compression test with Huffman algorithm...
	@$(TARGET) compress -i test\compress_test.txt -o build\compressed_haff.cmp -a haff
	@echo Running decompression test with Huffman algorithm...
	@$(TARGET) decompress -i build\compressed_haff.cmp -o build\decompressed_haff.txt
	@echo Verifying Huffman compression/decompression...
	@fc test\compress_test.txt build\decompressed_haff.txt && echo  HUFFMAN COMPRESS/DECOMPRESS: FILE MATCH || echo  HUFFMAN COMPRESS/DECOMPRESS: FILE DIFFER

# 加密功能测试
encrypt-test: $(TARGET)
	@echo Creating test directory...
	@mkdir test 2>nul || echo test directory exists
	@mkdir build 2>nul || echo build directory exists
	@echo "This is a test file for encryption. It contains some content to test the encryption algorithm." > test\encrypt_test.txt
	@echo Running AES encryption test...
	@$(TARGET) encrypt -i test\encrypt_test.txt -o build\encrypted_aes.enc -a aes -k 123456
	@echo Running AES decryption test...
	@$(TARGET) decrypt -i build\encrypted_aes.enc -o build\decrypted_aes.txt -a aes -k 123456
	@echo Verifying AES encryption/decryption...
	@fc test\encrypt_test.txt build\decrypted_aes.txt && echo  AES ENCRYPT/DECRYPT: FILE MATCH || echo  AES ENCRYPT/DECRYPT: FILE DIFFER
	@echo Running DES encryption test...
	@$(TARGET) encrypt -i test\encrypt_test.txt -o build\encrypted_des.enc -a des -k 123456
	@echo Running DES decryption test...
	@$(TARGET) decrypt -i build\encrypted_des.enc -o build\decrypted_des.txt -a des -k 123456
	@echo Verifying DES encryption/decryption...
	@fc test\encrypt_test.txt build\decrypted_des.txt && echo  DES ENCRYPT/DECRYPT: FILE MATCH || echo  DES ENCRYPT/DECRYPT: FILE DIFFER

# 打包功能测试
pack-test: $(TARGET)
	@echo Creating test directory...
	@mkdir test 2>nul || echo test directory exists
	@mkdir build 2>nul || echo build directory exists
	@echo "Test file 1 content" > test\file1.txt
	@echo "Test file 2 content" > test\file2.txt
	@mkdir test\subdir 2>nul || echo subdir exists
	@echo "File in subdirectory" > test\subdir\subfile.txt
	@echo Running tar pack test...
	@$(TARGET) backup -s test -t build\tar_backup -a tar
	@echo Running tar unpack test...
	@$(TARGET) restore -f build\tar_backup\backup.dat -t build\tar_restore
	@echo Verifying tar pack/unpack...
	@fc test\file1.txt build\tar_restore\file1.txt && echo  TAR PACK/UNPACK: FILE1 MATCH || echo  TAR PACK/UNPACK: FILE1 DIFFER
	@fc test\file2.txt build\tar_restore\file2.txt && echo  TAR PACK/UNPACK: FILE2 MATCH || echo  TAR PACK/UNPACK: FILE2 DIFFER
	@fc test\subdir\subfile.txt build\tar_restore\subdir\subfile.txt && echo  TAR PACK/UNPACK: SUBDIR FILE MATCH || echo  TAR PACK/UNPACK: SUBDIR FILE DIFFER

# 详细压缩测试
detailed-compress-test: $(TARGET)
	@echo Creating test directory...
	@mkdir test 2>nul || echo test directory exists
	@mkdir build 2>nul || echo build directory exists
	@echo "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA......" > test\repetitive.txt
	@echo Testing LZ77 compression on repetitive data...
	@$(TARGET) compress -i test\repetitive.txt -o build\repetitive_lz77.cmp -a lz77
	@$(TARGET) decompress -i build\repetitive_lz77.cmp -o build\repetitive_lz77_decomp.txt
	@echo Verifying LZ77 on repetitive data...
	@fc test\repetitive.txt build\repetitive_lz77_decomp.txt && echo  LZ77 REPETITIVE DATA: FILE MATCH || echo  LZ77 REPETITIVE DATA: FILE DIFFER
	@echo Testing Huffman compression on repetitive data...
	@$(TARGET) compress -i test\repetitive.txt -o build\repetitive_haff.cmp -a haff
	@$(TARGET) decompress -i build\repetitive_haff.cmp -o build\repetitive_haff_decomp.txt
	@echo Verifying Huffman on repetitive data...
	@fc test\repetitive.txt build\repetitive_haff_decomp.txt && echo  HUFFMAN REPETITIVE DATA: FILE MATCH || echo  HUFFMAN REPETITIVE DATA: FILE DIFFER

# 详细打包测试
detailed-pack-test: $(TARGET)
	@echo Creating test directory...
	@mkdir test 2>nul || echo test directory exists
	@mkdir build 2>nul || echo build directory exists
	@echo "File A content" > test\fileA.txt
	@echo "File B content" > test\fileB.txt
	@echo "File C content" > test\fileC.txt
	@mkdir test\dir1 2>nul || echo dir1 exists
	@mkdir test\dir2 2>nul || echo dir2 exists
	@echo "Nested file 1" > test\dir1\nested1.txt
	@echo "Nested file 2" > test\dir1\nested2.txt
	@echo "Another nested file" > test\dir2\nested3.txt
	@echo Testing tar pack with nested directories...
	@$(TARGET) backup -s test -t build\nested_backup -a tar
	@echo Testing tar unpack with nested directories...
	@$(TARGET) restore -f build\nested_backup\backup.dat -t build\nested_restore
	@echo Verifying nested directory pack/unpack...
	@fc test\fileA.txt build\nested_restore\fileA.txt && echo  NESTED TAR: FILEA MATCH || echo  NESTED TAR: FILEA DIFFER
	@fc test\dir1\nested1.txt build\nested_restore\dir1\nested1.txt && echo  NESTED TAR: NESTED1 MATCH || echo  NESTED TAR: NESTED1 DIFFER
	@fc test\dir2\nested3.txt build\nested_restore\dir2\nested3.txt && echo  NESTED TAR: NESTED3 MATCH || echo  NESTED TAR: NESTED3 DIFFER

# 综合测试（压缩+打包）
combo-test: $(TARGET)
	@echo Creating test directory...
	@mkdir test 2>nul || echo test directory exists
	@mkdir build 2>nul || echo build directory exists
	@echo "Test file for combined compression and packing" > test\combo_test.txt
	@echo "Another test file" > test\combo_test2.txt
	@mkdir test\subfolder 2>nul || echo subfolder exists
	@echo "File in subfolder" > test\subfolder\sub.txt
	@echo Testing backup with tar packing and lz77 compression...
	@$(TARGET) backup -s test -t build\combo_backup -a tar -c lz77
	@echo Testing restore with decompression and unpacking...
	@$(TARGET) restore -f build\combo_backup\backup.dat -t build\combo_restore
	@echo Verifying combined backup/restore...
	@fc test\combo_test.txt build\combo_restore\combo_test.txt && echo  COMBO BACKUP/RESTORE: FILE1 MATCH || echo  COMBO BACKUP/RESTORE: FILE1 DIFFER
	@fc test\subfolder\sub.txt build\combo_restore\subfolder\sub.txt && echo  COMBO BACKUP/RESTORE: SUBFOLDER FILE MATCH || echo  COMBO BACKUP/RESTORE: SUBFOLDER FILE DIFFER

# 加密备份测试
encrypt-backup-test: $(TARGET)
	@echo Creating test directory for encryption backup...
	@mkdir test 2>nul || echo test directory exists
	@mkdir build 2>nul || echo build directory exists
	@echo "Test file for encrypted backup" > test\encrypt_backup_test.txt
	@echo "Testing backup with tar packing, lz77 compression, and AES encryption..."
	@$(TARGET) backup -s test -t build\encrypt_backup -a tar -c lz77 -e aes 123456
	@echo "Testing restore with decryption, decompression, and unpacking..."
	@$(TARGET) restore -f build\encrypt_backup\backup.dat -t build\encrypt_restore -e aes 123456
	@echo "Verifying encrypted backup/restore..."
	@fc test\encrypt_backup_test.txt build\encrypt_restore\encrypt_backup_test.txt && echo  ENCRYPTED BACKUP/RESTORE: FILE MATCH || echo  ENCRYPTED BACKUP/RESTORE: FILE DIFFER

# 完整测试
full-test: test-basic compress-test encrypt-test pack-test detailed-compress-test detailed-pack-test combo-test encrypt-backup-test

combo-params-test: $(TARGET)
	@echo Creating test for parameter passing in combo operation...
	@mkdir test 2>nul || echo test directory exists
	@mkdir build 2>nul || echo build directory exists
	@echo "Test file for parameter test" > test\param_test.txt
	@mkdir test\param_sub 2>nul || echo param_sub directory exists
	@echo "Sub file for parameter test" > test\param_sub\sub.txt
	@echo "Testing backup with tar packing and lz77 compression (with debug)..."
	@$(TARGET) backup -s test -t build\param_backup -a tar -c lz77
	@echo "Backup completed. Checking backup file size:"
	@dir build\param_backup\backup.dat
	@echo "Testing restore..."
	@$(TARGET) restore -f build\param_backup\backup.dat -t build\param_restore
	@echo "Restore completed. Checking results:"
	@dir build\param_restore /s
	@echo "Verifying files:"
	@fc test\param_test.txt build\param_restore\param_test.txt && echo  PARAM TEST: ROOT FILE MATCH || echo  PARAM TEST: ROOT FILE DIFFER
	@if exist build\param_restore\param_sub (
		@if exist build\param_restore\param_sub\sub.txt (
			fc test\param_sub\sub.txt build\param_restore\param_sub\sub.txt && echo  PARAM TEST: SUB FILE MATCH || echo  PARAM TEST: SUB FILE DIFFER
		) else (
			echo  PARAM TEST: SUBFOLDER EXISTS BUT SUB.TXT MISSING
		)
	) else (
		echo  PARAM TEST: SUBFOLDER MISSING ENTIRELY
	)
.PHONY: all clean test-basic compress-test encrypt-test pack-test detailed-compress-test detailed-pack-test combo-test encrypt-backup-test full-test