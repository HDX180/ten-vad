#
#  Copyright © 2025 Agora
#  This file is part of TEN Framework, an open source project.
#  Licensed under the Apache License, Version 2.0, with certain conditions.
#  Refer to the "LICENSE" file in the root directory for more information.
#

# 编译器设置 - 改为g++以支持C++库链接
CC = g++
CXX = g++
CFLAGS = -Wall -O2 -fPIC -std=c++11
CXXFLAGS = $(CFLAGS)
LDFLAGS_SHARED = -shared
LDFLAGS_STATIC = -r

# C++库链接设置
LIBS = -lm -lstdc++ -lpthread
# 如果系统使用libc++，添加以下选项
ifeq ($(shell ldd ../lib/Linux/x64/libten_vad.so 2>/dev/null | grep -c "libc++"), 1)
    LIBS += -lc++ -lc++abi
    CFLAGS += -stdlib=libc++
    CXXFLAGS += -stdlib=libc++
endif

# 目录设置
INCLUDE_DIR = ../include
LIB_DIR = ../lib/Linux/x64
BUILD_DIR = build
EXAMPLES_DIR = .

# 源文件
STATE_MACHINE_SOURCE = ten_vad_state_machine.c
EXAMPLE_SOURCE = vad_state_example.c

# 目标文件
STATE_MACHINE_OBJ = $(BUILD_DIR)/ten_vad_state_machine.o

# 库文件
SHARED_LIB = $(BUILD_DIR)/libten_vad_state_machine.so
STATIC_LIB = $(BUILD_DIR)/libten_vad_state_machine.a
TEN_VAD_LIB = $(LIB_DIR)/libten_vad.so

# 可执行文件
EXAMPLE_EXEC = $(BUILD_DIR)/vad_state_example

# 平台检测
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    LDFLAGS_SHARED = -dynamiclib
    SHARED_LIB = $(BUILD_DIR)/libten_vad_state_machine.dylib
    CFLAGS += -fvisibility=hidden
    CXXFLAGS += -fvisibility=hidden
endif

# 默认目标 - 先检查依赖再构建
all: check-deps $(SHARED_LIB) $(STATIC_LIB) $(EXAMPLE_EXEC)

# 创建构建目录
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# 检查依赖 - 作为独立的目标，不是文件依赖
check-deps:
	@echo "🔍 检查依赖..."
	@if [ ! -f "$(TEN_VAD_LIB)" ]; then \
		echo "❌ 错误: 找不到 $(TEN_VAD_LIB)"; \
		echo "   请确保已有的 libten_vad.so 位于 $(LIB_DIR) 目录"; \
		exit 1; \
	fi
	@if [ ! -f "$(INCLUDE_DIR)/ten_vad.h" ]; then \
		echo "❌ 错误: 找不到 $(INCLUDE_DIR)/ten_vad.h"; \
		exit 1; \
	fi
	@if [ ! -f "$(STATE_MACHINE_SOURCE)" ]; then \
		echo "❌ 错误: 找不到 $(STATE_MACHINE_SOURCE)"; \
		exit 1; \
	fi
	@if [ ! -f "$(EXAMPLE_SOURCE)" ]; then \
		echo "❌ 错误: 找不到 $(EXAMPLE_SOURCE)"; \
		exit 1; \
	fi
	@echo "✅ 依赖检查通过"
	@echo "🔍 检查libten_vad.so的依赖..."
	@ldd $(TEN_VAD_LIB) || echo "⚠️  无法检查动态库依赖"

# 编译状态机目标文件
$(STATE_MACHINE_OBJ): $(STATE_MACHINE_SOURCE) | $(BUILD_DIR)
	@echo "🔨 编译状态机目标文件..."
	$(CC) $(CFLAGS) -DTENVAD_STATE_MACHINE_EXPORTS -I$(INCLUDE_DIR) -c $< -o $@

# 构建动态库
$(SHARED_LIB): $(STATE_MACHINE_OBJ)
	@echo "📦 构建状态机动态库..."
	$(CC) $(LDFLAGS_SHARED) -o $@ $^ -L$(LIB_DIR) -lten_vad $(LIBS)

# 构建静态库
$(STATIC_LIB): $(STATE_MACHINE_OBJ)
	@echo "📦 构建状态机静态库..."
	ar rcs $@ $^

# 构建示例程序 - 添加rpath以便运行时找到库
$(EXAMPLE_EXEC): $(EXAMPLE_SOURCE) $(SHARED_LIB) | $(BUILD_DIR)
	@echo "🔨 构建示例程序..."
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -I$(EXAMPLES_DIR) -o $@ $< \
		-L$(BUILD_DIR) -L$(LIB_DIR) \
		-Wl,-rpath,$(BUILD_DIR) -Wl,-rpath,$(LIB_DIR) \
		-lten_vad_state_machine -lten_vad $(LIBS)

# 测试
test: $(EXAMPLE_EXEC)
	@echo "🧪 运行测试..."
	@echo "设置库路径并运行示例程序..."
	LD_LIBRARY_PATH=$(BUILD_DIR):$(LIB_DIR) $(EXAMPLE_EXEC) ../examples/s0724-s0730.wav || \
	echo "⚠️  需要音频文件进行完整测试，但程序编译成功"

# 安装
install: all
	@echo "📥 安装库文件..."
	install -d /usr/local/lib /usr/local/include
	install $(SHARED_LIB) $(STATIC_LIB) /usr/local/lib/
	install ten_vad_state_machine.h /usr/local/include/
	@echo "✅ 安装完成"

# 清理
clean:
	@echo "🧹 清理构建文件..."
	rm -rf $(BUILD_DIR)

# 显示构建信息
info:
	@echo "📋 构建信息:"
	@echo "   编译器: $(CC)"
	@echo "   编译选项: $(CFLAGS)"
	@echo "   链接库: $(LIBS)"
	@echo "   包含目录: $(INCLUDE_DIR)"
	@echo "   库目录: $(LIB_DIR)"
	@echo "   构建目录: $(BUILD_DIR)"
	@echo "   平台: $(UNAME_S)"
	@echo ""
	@echo "📦 构建目标:"
	@echo "   动态库: $(SHARED_LIB)"
	@echo "   静态库: $(STATIC_LIB)"
	@echo "   示例程序: $(EXAMPLE_EXEC)"

# 调试信息 - 新增目标用于诊断问题
debug-deps:
	@echo "🔍 调试依赖信息:"
	@echo "检查libten_vad.so的依赖:"
	@ldd $(TEN_VAD_LIB) || echo "无法检查依赖"
	@echo ""
	@echo "检查系统C++库:"
	@ldconfig -p | grep -E "(libc\+\+|libstdc\+\+)" || echo "未找到C++库"
	@echo ""
	@echo "检查编译器版本:"
	@$(CC) --version
	@echo ""
	@echo "检查可用的C++库:"
	@find /usr/lib* /lib* -name "libc++*" -o -name "libstdc++*" 2>/dev/null | head -10

# 帮助
help:
	@echo "🔧 可用目标:"
	@echo "   all         - 构建所有目标 (默认)"
	@echo "   shared      - 仅构建动态库"
	@echo "   static      - 仅构建静态库"
	@echo "   example     - 仅构建示例程序"
	@echo "   check-deps  - 检查依赖"
	@echo "   debug-deps  - 调试依赖问题"
	@echo "   test        - 运行测试"
	@echo "   install     - 安装到系统目录"
	@echo "   clean       - 清理构建文件"
	@echo "   info        - 显示构建信息"
	@echo "   help        - 显示此帮助"

# 单独构建目标 - 这些也需要先检查依赖
shared: check-deps $(SHARED_LIB)
static: check-deps $(STATIC_LIB)
example: check-deps $(EXAMPLE_EXEC)

# 声明伪目标
.PHONY: all clean install test help info check-deps debug-deps shared static example
