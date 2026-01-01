# 编译器
CC = gcc

# 编译选项
CFLAGS = -Wall -Iinclude -g

# 链接选项
LDFLAGS = -lws2_32 -lcrypt32

# 目标文件
TARGET = backup_software

# 源文件
SRCS = src/main.c src/backup.c src/restore.c src/filter.c src/pack.c src/compress.c src/encrypt.c src/metadata.c src/huffman.c src/traverse.c

# 目标文件 - 输出到build目录
OBJS = $(patsubst src/%.c,build/%.o,$(SRCS))

# 默认目标
all: $(TARGET)

# 编译目标
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

# 编译源文件 - 输出到build目录
build/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# 清理目标
clean:
	rm -f $(OBJS) $(TARGET)

# 测试目标
test:
	@echo "测试备份功能..."
	@mkdir test_source 2>nul || echo 目录已存在
	@mkdir test_backup 2>nul || echo 目录已存在
	@echo "test file content" > test_source/test.txt
	@./$(TARGET) backup -s test_source -t test_backup
	@echo "测试完成！"

# 运行目标
run:
	./$(TARGET)

.PHONY: all clean test run
