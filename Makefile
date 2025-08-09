# 简单的Makefile构建文件
# 这是CMake的替代方案，适用于简单的编译需求

# 编译器设置
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2

# Android NDK交叉编译设置（如果需要）
ifdef ANDROID_NDK
    ifeq ($(ANDROID_ABI),arm64-v8a)
        CXX = $(ANDROID_NDK)/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android21-clang++
    else ifeq ($(ANDROID_ABI),armeabi-v7a)
        CXX = $(ANDROID_NDK)/toolchains/llvm/prebuilt/linux-x86_64/bin/armv7a-linux-androideabi21-clang++
    else ifeq ($(ANDROID_ABI),x86_64)
        CXX = $(ANDROID_NDK)/toolchains/llvm/prebuilt/linux-x86_64/bin/x86_64-linux-android21-clang++
    else ifeq ($(ANDROID_ABI),x86)
        CXX = $(ANDROID_NDK)/toolchains/llvm/prebuilt/linux-x86_64/bin/i686-linux-android21-clang++
    endif
    CXXFLAGS += -fPIC -DANDROID_PLATFORM
endif

# 目标文件
TARGET = disk_bandwidth_test
SOURCE = test.cpp
BUILDDIR = build
BINDIR = $(BUILDDIR)/bin

# 默认目标
all: $(BINDIR)/$(TARGET)

# 创建构建目录
$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BINDIR):
	mkdir -p $(BINDIR)

# 编译目标
$(BINDIR)/$(TARGET): $(SOURCE) | $(BUILDDIR) $(BINDIR)
	$(CXX) $(CXXFLAGS) -o $@ $<
	@echo "构建完成: $@"

# 清理
clean:
	rm -rf $(BUILDDIR)
	@echo "清理完成"

# 安装（复制到系统路径）
install: $(BINDIR)/$(TARGET)
	cp $(BINDIR)/$(TARGET) /usr/local/bin/
	@echo "已安装到 /usr/local/bin/$(TARGET)"

# Android构建快捷方式
android-arm64:
	$(MAKE) ANDROID_NDK=$(ANDROID_NDK) ANDROID_ABI=arm64-v8a

android-arm:
	$(MAKE) ANDROID_NDK=$(ANDROID_NDK) ANDROID_ABI=armeabi-v7a

android-x86_64:
	$(MAKE) ANDROID_NDK=$(ANDROID_NDK) ANDROID_ABI=x86_64

android-x86:
	$(MAKE) ANDROID_NDK=$(ANDROID_NDK) ANDROID_ABI=x86

# 调试构建
debug: CXXFLAGS += -g -DDEBUG
debug: $(BINDIR)/$(TARGET)

# 运行测试
test: $(BINDIR)/$(TARGET)
	./$(BINDIR)/$(TARGET) /tmp 10

# 显示帮助
help:
	@echo "Android磁盘带宽测试工具 - Makefile构建"
	@echo ""
	@echo "可用目标:"
	@echo "  all              - 构建项目（默认）"
	@echo "  clean            - 清理构建文件"
	@echo "  debug            - 调试构建"
	@echo "  test             - 构建并运行测试"
	@echo "  install          - 安装到系统"
	@echo "  help             - 显示此帮助"
	@echo ""
	@echo "Android构建:"
	@echo "  android-arm64    - 构建arm64-v8a版本"
	@echo "  android-arm      - 构建armeabi-v7a版本"
	@echo "  android-x86_64   - 构建x86_64版本"
	@echo "  android-x86      - 构建x86版本"
	@echo ""
	@echo "Android构建示例:"
	@echo "  export ANDROID_NDK=/path/to/ndk"
	@echo "  make android-arm64"
	@echo ""
	@echo "本地构建示例:"
	@echo "  make"
	@echo "  make debug"
	@echo "  make test"

.PHONY: all clean install debug test help android-arm64 android-arm android-x86_64 android-x86
