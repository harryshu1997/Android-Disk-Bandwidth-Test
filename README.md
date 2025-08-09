# Android磁盘带宽测试工具

这是一个用于测试Android设备磁盘I/O性能的C++工具，可以测量顺序写入、顺序读取和随机读取的带宽性能。

## 功能特性

- **顺序写入测试**: 测量大文件顺序写入的带宽
- **顺序读取测试**: 测量大文件顺序读取的带宽  
- **随机读取测试**: 测量小块随机读取的带宽和IOPS
- **跨平台支持**: 支持Linux和Android平台
- **可配置参数**: 支持自定义测试目录和文件大小

## 构建要求

### 本地Linux构建
- CMake 3.10+
- GCC或Clang编译器
- C++11支持

### Android构建
- Android NDK
- CMake 3.10+
- Android API Level 21+

## 快速开始

### 1. 本地Linux构建

```bash
# 使用构建脚本（推荐）
./build.sh

# 或手动构建
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### 2. Android构建

```bash
# 设置Android NDK路径
export ANDROID_NDK_ROOT=/path/to/your/android-ndk

# 构建（默认arm64-v8a）
./build.sh --android

# 或指定NDK路径
./build.sh --android --ndk-path /path/to/android-ndk

# 构建其他架构
./build.sh --android --abi armeabi-v7a
./build.sh --android --abi x86_64
```

### 3. 部署到Android设备

```bash
# 推送到设备
adb push build/bin/disk_bandwidth_test /data/local/tmp/

# 添加执行权限
adb shell chmod 755 /data/local/tmp/disk_bandwidth_test
```

## 使用方法

### Android存储目录选择

在Android设备上，不同的存储目录有不同的特性：

#### 推荐的测试目录：

1. **`/sdcard`（默认）** - 用户存储分区
   - ✅ 大多数应用都能访问
   - ✅ 代表真实用户文件存储性能
   - ❌ 需要存储权限
   - ❌ 可能受其他应用影响

2. **`/data/local/tmp`** - 系统临时存储
   - ✅ 通常是最快的存储
   - ✅ 不受用户应用影响
   - ❌ 需要root权限或通过adb访问
   - ✅ 适合基准测试

3. **`/data/data/com.yourapp`** - 应用私有存储
   - ✅ 应用专用，不需要额外权限
   - ✅ 代表应用数据存储性能
   - ❌ 路径因应用而异

4. **`/cache`** - 缓存分区
   - ⚠️ 可能在不同设备上表现不同
   - ❌ 需要特殊权限

### 存储类型说明：
- `/sdcard` → 内部存储的用户分区（不是真正的SD卡）
- `/data/local/tmp` → 系统数据分区，通常性能最好
- 外置SD卡路径通常在 `/storage/` 下

### 本地运行
```bash
# 默认参数（/tmp目录，1GB文件）
./build/bin/disk_bandwidth_test

# 自定义目录和文件大小
  $EXECUTABLE [目录] [文件大小MB]"
echo "  例如: $EXECUTABLE /tmp 1024"
```

### Android设备运行
```bash
# 默认用户存储测试（/sdcard目录，1GB文件）
adb shell /data/local/tmp/disk_bandwidth_test

# 系统存储测试（通常更快）
adb shell /data/local/tmp/disk_bandwidth_test /data/local/tmp 1024

# 自定义参数
adb shell /data/local/tmp/disk_bandwidth_test /sdcard 2048

# 比较不同存储性能
adb shell /data/local/tmp/disk_bandwidth_test /sdcard 512       # 用户存储
adb shell /data/local/tmp/disk_bandwidth_test /data/local/tmp 512  # 系统存储
```

### 性能对比建议
```bash
# 完整性能对比脚本
echo "=== Android存储性能对比 ==="

echo "1. 用户存储 (/sdcard):"
adb shell /data/local/tmp/disk_bandwidth_test /sdcard 1024

echo "2. 系统存储 (/data/local/tmp):"
adb shell /data/local/tmp/disk_bandwidth_test /data/local/tmp 1024

echo "3. 如果有外置SD卡:"
adb shell /data/local/tmp/disk_bandwidth_test /storage/[external_sd] 1024
```

## 命令行参数

```
Usage: disk_bandwidth_test [test_directory] [file_size_mb]

参数：
- test_directory: 测试目录路径（默认：/sdcard）
- file_size_mb: 测试文件大小，单位MB（默认：1024）

示例：
disk_bandwidth_test /sdcard 1024
disk_bandwidth_test /data/local/tmp 2048
```

## 构建脚本选项

```bash
./build.sh [选项]

选项:
  -h, --help              显示帮助信息
  -d, --debug             调试构建 (默认: Release)
  -a, --android           为Android构建
  -n, --ndk-path PATH     Android NDK路径
  -b, --abi ABI           Android ABI (默认: arm64-v8a)
                          可选: arm64-v8a, armeabi-v7a, x86, x86_64
  -p, --platform LEVEL    Android平台级别 (默认: android-21)
```

## 输出示例 Oneplus12

```
Simple Disk Bandwidth Test for Android
Usage: ./disk_bandwidth_test [test_directory] [file_size_mb]
Example: ./disk_bandwidth_test /data/local/tmp/bandwidth 2048


========================================
     Simple Disk Bandwidth Test
========================================
Test directory: /data/local/tmp/bandwidth
Test file size: 2048 MB

=== Sequential Write Test ===
File size: 2048 MB
Write bandwidth: 1911.76 MB/s

=== Sequential Read Test ===
File size: 2048.00 MB
Read bandwidth: 8093.70 MB/s

=== Random Read Test ===
Number of random reads: 1000
sh: can't create /proc/sys/vm/drop_caches: Permission denied
Successful reads: 1000/1000
Random read bandwidth: 1048.91 MB/s
Random read IOPS: 268520
Warning: Random read speed seems unrealistically high (possible cache effect)

=== Random Write Test ===
Number of random writes: 1000
Successful writes: 1000/1000
Random write bandwidth: 55.60 MB/s
Random write IOPS: 14235

========================================
              SUMMARY
========================================
Sequential Write: 1911.76 MB/s
Sequential Read:  8093.70 MB/s
Random Read:      1048.91 MB/s
Random Write:     55.60 MB/s
========================================
```

## 注意事项

1. **权限要求**: 在Android设备上运行可能需要root权限才能清除缓存
2. **存储空间**: 确保测试目录有足够的可用空间
3. **性能影响**: 测试过程中会进行大量I/O操作，可能影响设备性能
4. **临时文件**: 程序会创建临时测试文件，程序结束时会自动清理

## 故障排除

### 权限错误
```bash
# 确保有写入权限
adb shell ls -la /data/local/tmp/
adb shell chmod 755 /data/local/tmp/disk_bandwidth_test
```

### 存储空间不足
```bash
# 检查可用空间
adb shell df -h /sdcard
# 或使用更小的测试文件
adb shell /data/local/tmp/disk_bandwidth_test /sdcard 50
```