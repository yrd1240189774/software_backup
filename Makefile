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
SRCDIR = src
SOURCES = $(SRCDIR)/main.c $(SRCDIR)/backup.c $(SRCDIR)/restore.c $(SRCDIR)/compress.c \
          $(SRCDIR)/encrypt.c $(SRCDIR)/pack.c $(SRCDIR)/traverse.c $(SRCDIR)/filter.c \
          $(SRCDIR)/huffman.c $(SRCDIR)/metadata.c $(SRCDIR)/schedule.c $(SRCDIR)/cleanup.c

# 目标文件 - 输出到build目录
OBJS = $(patsubst src/%.c,build/%.o,$(SOURCES))

# 默认目标
all: $(TARGET)

# 编译目标
$(TARGET): build $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET).exe $(OBJS) $(LDFLAGS)

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

# 测试目标 - 帮助信息测试
test-help:
	@echo === ²âÊÔ°ïÖúÐÅÏ¢ ===
	-./$(TARGET).exe

# 测试基础功能
test-basic:
	@echo === ²âÊÔ»ù´¡±¸·Ý¹¦ÄÜ ===
	@mkdir test_source 2>nul || echo test_source directory exists
	@echo test data for basic backup > test_source\test_basic.txt
	-./$(TARGET).exe backup -s test_source -t test_backup_basic

# 测试数据清理功能 - 创建多个备份文件
test-cleanup:
	@echo === ²âÊÔÊý¾ÝÇåÀí¹¦ÄÜ ===
	@mkdir test_cleanup_dir 2>nul || echo test_cleanup_dir directory exists
	@echo test data for cleanup 1 > test_cleanup_dir\cleanup_test1.txt
	@echo test data for cleanup 2 > test_cleanup_dir\cleanup_test2.txt
	@echo "³õÊ¼±¸·Ý"
	-./$(TARGET).exe backup -s test_cleanup_dir -t test_backup_cleanup_1
	@echo "±¸·Ý2"
	-./$(TARGET).exe backup -s test_cleanup_dir -t test_backup_cleanup_2
	@echo "±¸·Ý3"
	-./$(TARGET).exe backup -s test_cleanup_dir -t test_backup_cleanup_3
	@echo "±¸·Ý4"
	-./$(TARGET).exe backup -s test_cleanup_dir -t test_backup_cleanup_4
	@echo "±¸·Ý5"
	-./$(TARGET).exe backup -s test_cleanup_dir -t test_backup_cleanup_5
	@echo "²âÊÔÊý¾ÝÇåÀí£¬±£Áô×îÐÂ3¸ö±¸·Ý"
	@echo "½«Îå¸ö±¸·ÝÖØÃüÃûµ½Ò»¸öÄ¿Â¼²¢²âÊÔÇåÀí"
	@mkdir test_cleanup_multi 2>nul || echo test_cleanup_multi directory exists
	copy test_backup_cleanup_1\backup.dat test_cleanup_multi\backup_1.dat 2>nul || echo backup_1.dat created
	copy test_backup_cleanup_2\backup.dat test_cleanup_multi\backup_2.dat 2>nul || echo backup_2.dat created
	copy test_backup_cleanup_3\backup.dat test_cleanup_multi\backup_3.dat 2>nul || echo backup_3.dat created
	copy test_backup_cleanup_4\backup.dat test_cleanup_multi\backup_4.dat 2>nul || echo backup_4.dat created
	copy test_backup_cleanup_5\backup.dat test_cleanup_multi\backup_5.dat 2>nul || echo backup_5.dat created
	-./$(TARGET).exe cleanup -d test_cleanup_multi --keep-days 1 --max-count 3

# 测试定时备份功能（短时间间隔）
test-schedule:
	@echo === ²âÊÔ¶¨Ê±±¸·Ý¹¦ÄÜ ===
	@mkdir test_schedule_dir 2>nul || echo test_schedule_dir directory exists
	@echo test data for schedule 1 > test_schedule_dir\schedule_test1.txt
	@echo test data for schedule 2 > test_schedule_dir\schedule_test2.txt
	@echo test data for schedule 3 > test_schedule_dir\schedule_test3.txt
	@echo ×¢Òâ£º¶¨Ê±±¸·Ý²âÊÔ»áÔËÐÐÒ»¶ÎÊ±¼ä£¬Ã¿1·ÖÖÓÖ´ÐÐÒ»´Î±¸·Ý£¬×Ü¹²Ö´ÐÐ3´Î
	@echo °´ Ctrl+C ¿ÉÒÔÖÐ¶Ï²âÊÔ
	-./$(TARGET).exe schedule -s test_schedule_dir -t test_backup_schedule -i 1 --keep-days 1 --max-count 3

# 测试所有功能
test-all: test-help test-basic test-cleanup
	@echo === ËùÓÐ²âÊÔÍê³É ===

# 运行综合测试
full-test: all
	@echo === ¿ªÊ¼×ÛºÏ²âÊÔ ===
	@echo 1. ²âÊÔ°ïÖúÐÅÏ¢
	-./$(TARGET).exe
	@echo 2. ²âÊÔ±¸·Ý¹¦ÄÜ
	@mkdir test_data 2>nul || echo test_data directory exists
	@echo test content > test_data\test.txt
	-./$(TARGET).exe backup -s test_data -t test_output
	@echo 3. ²âÊÔ»¹Ô­¹¦ÄÜ
	@mkdir test_restore 2>nul || echo test_restore directory exists
	@echo ±¸·Ý»¹Ô­²âÊÔ´ýÊµÏÖ...
	@echo 4. ²âÊÔÑ¹Ëõ¹¦ÄÜ
	@echo compress test data > test_compress_input.txt
	-./$(TARGET).exe compress -i test_compress_input.txt -o test_compress_output.cmp -a haff
	@echo 5. ²âÊÔ½âÑ¹¹¦ÄÜ
	-./$(TARGET).exe decompress -i test_compress_output.cmp -o test_decompress_output.txt
	@echo 6. ²âÊÔÊý¾ÝÇåÀí
	-./$(TARGET).exe cleanup -d test_output --keep-days 1 --max-count 5
	@echo === ×ÛºÏ²âÊÔÍê³É ===


.PHONY: all clean test-help test-basic test-cleanup test-schedule test-all full-test test-schedule-background