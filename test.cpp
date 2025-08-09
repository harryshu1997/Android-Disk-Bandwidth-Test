#include <iostream>
#include <vector>
#include <chrono>
#include <iomanip>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <random>
#include <algorithm>
#include <numeric>
#include <sys/statvfs.h>
#include <cstring>
#include <errno.h>

class DiskBandwidthTest {
private:
    std::string test_dir;
    std::string test_file;
    std::mt19937 rng;
    
    // Test parameters
    static constexpr size_t KB = 1024;
    static constexpr size_t MB = 1024 * KB;
    static constexpr size_t DEFAULT_FILE_SIZE = 2048 * MB;  // 2 GB default
    static constexpr size_t BUFFER_SIZE = 4 * MB;         // 4 MB buffer
    
    double bytes_to_mb(size_t bytes) {
        return static_cast<double>(bytes) / MB;
    }
    
    double calculate_bandwidth_mbps(size_t bytes, double time_seconds) {
        return (bytes_to_mb(bytes) / time_seconds);
    }
    
    // Create test directory if it doesn't exist
    void ensure_test_directory() {
        struct stat st;
        if (stat(test_dir.c_str(), &st) != 0) {
            // Try to create the directory
            std::string mkdir_cmd = "mkdir -p " + test_dir;
            system(mkdir_cmd.c_str());
        }
    }
    
public:
    DiskBandwidthTest(const std::string& dir = "/data/local/tmp/bandwidth") 
        : test_dir(dir), test_file(dir + "/test.dat"), 
          rng(std::chrono::steady_clock::now().time_since_epoch().count()) {
        ensure_test_directory();
    }
    
    ~DiskBandwidthTest() {
        unlink(test_file.c_str());
    }
    
    // Simple sequential write test
    double test_sequential_write(size_t file_size = DEFAULT_FILE_SIZE) {
        std::cout << "\n=== Sequential Write Test ===\n";
        std::cout << "File size: " << bytes_to_mb(file_size) << " MB\n";
        
        std::vector<char> buffer(BUFFER_SIZE);
        // Fill buffer with test pattern
        for (size_t i = 0; i < BUFFER_SIZE; ++i) {
            buffer[i] = static_cast<char>(i % 256);
        }
        
        int fd = open(test_file.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if (fd < 0) {
            std::cerr << "Error: Cannot create test file\n";
            return 0;
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        
        size_t bytes_written = 0;
        while (bytes_written < file_size) {
            size_t to_write = std::min(BUFFER_SIZE, file_size - bytes_written);
            ssize_t written = write(fd, buffer.data(), to_write);
            if (written < 0) {
                std::cerr << "Write error\n";
                close(fd);
                return 0;
            }
            bytes_written += written;
        }
        
        fsync(fd);
        auto end = std::chrono::high_resolution_clock::now();
        close(fd);
        
        std::chrono::duration<double> diff = end - start;
        double bandwidth = calculate_bandwidth_mbps(bytes_written, diff.count());
        
        std::cout << "Write bandwidth: " << std::fixed << std::setprecision(2) 
                  << bandwidth << " MB/s\n";
        return bandwidth;
    }
    
    // Simple sequential read test
    double test_sequential_read(size_t file_size = DEFAULT_FILE_SIZE) {
        std::cout << "\n=== Sequential Read Test ===\n";
        std::cout << "File size: " << bytes_to_mb(file_size) << " MB\n";
        
        // Create test file if needed
        struct stat st;
        if (stat(test_file.c_str(), &st) != 0) {
            std::cout << "Creating test file...\n";
            test_sequential_write(file_size);
        }
        
        std::vector<char> buffer(BUFFER_SIZE);
        
        int fd = open(test_file.c_str(), O_RDONLY);
        if (fd < 0) {
            std::cerr << "Error: Cannot open test file for reading\n";
            return 0;
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        
        size_t bytes_read = 0;
        ssize_t read_bytes;
        while ((read_bytes = read(fd, buffer.data(), BUFFER_SIZE)) > 0) {
            bytes_read += read_bytes;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        close(fd);
        
        std::chrono::duration<double> diff = end - start;
        double bandwidth = calculate_bandwidth_mbps(bytes_read, diff.count());
        
        std::cout << "Read bandwidth: " << std::fixed << std::setprecision(2) 
                  << bandwidth << " MB/s\n";
        return bandwidth;
    }
    
    // Simple random read test
    double test_random_read(size_t file_size = DEFAULT_FILE_SIZE, size_t num_reads = 1000) {
        std::cout << "\n=== Random Read Test ===\n";
        std::cout << "Number of random reads: " << num_reads << "\n";
        
        // Ensure test file exists
        struct stat st;
        if (stat(test_file.c_str(), &st) != 0) {
            std::cout << "Creating test file...\n";
            test_sequential_write(file_size);
        }
        
        // Try to clear cache before random access test
        sync();
        system("echo 3 > /proc/sys/vm/drop_caches 2>/dev/null");
        
        int fd = open(test_file.c_str(), O_RDONLY);
        if (fd < 0) {
            std::cerr << "Error: Cannot open test file for random reading\n";
            return 0;
        }
        
        const size_t block_size = 4 * KB;
        std::vector<char> buffer(block_size);
        
        // Use larger range to avoid cache hits - ensure we're accessing different parts of file
        const size_t max_blocks = file_size / block_size;
        const size_t stride = std::max(size_t(1), max_blocks / num_reads); // Spread accesses across file
        
        std::uniform_int_distribution<size_t> dist(0, max_blocks - 1);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        size_t bytes_read = 0;
        size_t successful_reads = 0;
        
        for (size_t i = 0; i < num_reads; ++i) {
            // Generate truly random offset, avoiding clustering
            off_t offset = (dist(rng) * stride) % max_blocks;
            offset *= block_size;
            
            if (lseek(fd, offset, SEEK_SET) == -1) {
                continue;
            }
            
            ssize_t read_bytes = read(fd, buffer.data(), block_size);
            if (read_bytes > 0) {
                bytes_read += read_bytes;
                successful_reads++;
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        close(fd);
        
        if (successful_reads == 0) {
            std::cerr << "No successful random reads\n";
            return 0;
        }
        
        std::chrono::duration<double> diff = end - start;
        double bandwidth = calculate_bandwidth_mbps(bytes_read, diff.count());
        double iops = successful_reads / diff.count();
        
        std::cout << "Successful reads: " << successful_reads << "/" << num_reads << "\n";
        std::cout << "Random read bandwidth: " << std::fixed << std::setprecision(2) 
                  << bandwidth << " MB/s\n";
        std::cout << "Random read IOPS: " << std::fixed << std::setprecision(0) << iops << "\n";
        
        // Sanity check - random read should be much slower than sequential
        if (bandwidth > 500.0) {
            std::cout << "Warning: Random read speed seems unrealistically high (possible cache effect)\n";
        }
        
        return bandwidth;
    }
    
    // Simple random write test
    double test_random_write(size_t file_size = DEFAULT_FILE_SIZE, size_t num_writes = 1000) {
        std::cout << "\n=== Random Write Test ===\n";
        std::cout << "Number of random writes: " << num_writes << "\n";
        
        // Ensure test file exists (create if needed)
        struct stat st;
        if (stat(test_file.c_str(), &st) != 0) {
            std::cout << "Creating test file...\n";
            test_sequential_write(file_size);
        }
        
        // Try to open with O_SYNC to bypass write cache for more accurate measurement
        int fd = open(test_file.c_str(), O_WRONLY | O_SYNC);
        if (fd < 0) {
            // Fallback to regular open if O_SYNC is not supported
            fd = open(test_file.c_str(), O_WRONLY);
            if (fd < 0) {
                std::cerr << "Error: Cannot open test file for random writing\n";
                return 0;
            }
        }
        
        const size_t block_size = 4 * KB;
        std::vector<char> buffer(block_size);
        
        // Fill buffer with varying test pattern for each write
        auto fill_buffer = [&buffer, &block_size](size_t iteration) {
            for (size_t i = 0; i < block_size; ++i) {
                buffer[i] = static_cast<char>((i + iteration * 37 + 0xAA) % 256);
            }
        };
        
        // Use larger range to avoid clustering writes
        const size_t max_blocks = file_size / block_size;
        const size_t stride = std::max(size_t(1), max_blocks / num_writes);
        
        std::uniform_int_distribution<size_t> dist(0, max_blocks - 1);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        size_t bytes_written = 0;
        size_t successful_writes = 0;
        
        for (size_t i = 0; i < num_writes; ++i) {
            // Generate truly random offset with better distribution
            off_t offset = (dist(rng) * stride) % max_blocks;
            offset *= block_size;
            
            // Update buffer content for each write
            fill_buffer(i);
            
            if (lseek(fd, offset, SEEK_SET) == -1) {
                continue;
            }
            
            ssize_t written = write(fd, buffer.data(), block_size);
            if (written > 0) {
                bytes_written += written;
                successful_writes++;
            }
        }
        
        // Force sync only once at the end for more accurate timing
        fsync(fd);
        auto end = std::chrono::high_resolution_clock::now();
        close(fd);
        
        if (successful_writes == 0) {
            std::cerr << "No successful random writes\n";
            return 0;
        }
        
        std::chrono::duration<double> diff = end - start;
        double bandwidth = calculate_bandwidth_mbps(bytes_written, diff.count());
        double iops = successful_writes / diff.count();
        
        std::cout << "Successful writes: " << successful_writes << "/" << num_writes << "\n";
        std::cout << "Random write bandwidth: " << std::fixed << std::setprecision(2) 
                  << bandwidth << " MB/s\n";
        std::cout << "Random write IOPS: " << std::fixed << std::setprecision(0) << iops << "\n";
        
        // Sanity check - random write should be much slower than sequential
        if (bandwidth > 1000.0) {
            std::cout << "Warning: Random write speed seems unrealistically high (possible cache effect)\n";
        }
        
        return bandwidth;
    }
    
    // Run all tests
    void run_all_tests(size_t file_size = DEFAULT_FILE_SIZE) {
        std::cout << "\n========================================\n";
        std::cout << "     Simple Disk Bandwidth Test\n";
        std::cout << "========================================\n";
        std::cout << "Test directory: " << test_dir << "\n";
        std::cout << "Test file size: " << bytes_to_mb(file_size) << " MB\n";
        
        double write_bw = test_sequential_write(file_size);
        double read_bw = test_sequential_read(file_size);
        double random_read_bw = test_random_read(file_size, 1000);
        double random_write_bw = test_random_write(file_size, 1000);
        
        std::cout << "\n========================================\n";
        std::cout << "              SUMMARY\n";
        std::cout << "========================================\n";
        std::cout << "Sequential Write: " << std::fixed << std::setprecision(2) 
                  << write_bw << " MB/s\n";
        std::cout << "Sequential Read:  " << std::fixed << std::setprecision(2) 
                  << read_bw << " MB/s\n";
        std::cout << "Random Read:      " << std::fixed << std::setprecision(2) 
                  << random_read_bw << " MB/s\n";
        std::cout << "Random Write:     " << std::fixed << std::setprecision(2) 
                  << random_write_bw << " MB/s\n";
        std::cout << "========================================\n";
    }
};

int main(int argc, char* argv[]) {
    std::string test_dir = "/data/local/tmp/bandwidth";  // Default: Fast system storage
    size_t file_size = 2048ULL * 1024 * 1024;  // 2 GB default
    
    std::cout << "Simple Disk Bandwidth Test for Android\n";
    std::cout << "Usage: " << argv[0] << " [test_directory] [file_size_mb]\n";
    std::cout << "Example: " << argv[0] << " /data/local/tmp/bandwidth 2048\n\n";
    
    if (argc > 1) {
        test_dir = argv[1];
    }
    if (argc > 2) {
        long long size_mb = std::atoll(argv[2]);
        if (size_mb <= 0 || size_mb > 10000) {
            std::cerr << "Error: File size must be between 1 and 10000 MB\n";
            return 1;
        }
        file_size = size_mb * 1024 * 1024;
    }
    
    try {
        DiskBandwidthTest test(test_dir);
        test.run_all_tests(file_size);
    } catch (...) {
        std::cerr << "Error: Test failed\n";
        return 1;
    }
    
    return 0;
}