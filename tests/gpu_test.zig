const std = @import("std");
const c = @cImport({
    @cInclude("Search.h");
});

test "CUDA Search Test" {
    const fs = std.fs;
    const allocator = std.testing.allocator;
    
    // Create a test directory
    const test_dir = "test_files";
    try fs.cwd().makeDir(test_dir);
    defer fs.cwd().deleteTree(test_dir) catch {};
    
    // Create test files with known content
    const test_files = [_]struct {
        name: []const u8,
        content: []const u8,
    }{
        .{ .name = "file1.txt", .content = "Hello World\n" },
        .{ .name = "file2.txt", .content = "This is a test file\n" },
        .{ .name = "file3.txt", .content = "Another test with World in it\n" },
    };
    
    for (test_files) |file| {
        const full_path = try fs.path.join(allocator, &[_][]const u8{ test_dir, file.name });
        defer allocator.free(full_path);
        
        const f = try fs.cwd().createFile(full_path, .{});
        defer f.close();
        
        try f.writeAll(file.content);
    }
    
    // Define test cases
    const test_cases = [_]struct {
        pattern: []const u8,
        expected: bool,
    }{
        .{ .pattern = "World", .expected = true },
        .{ .pattern = "NotFound", .expected = false },
        .{ .pattern = "test", .expected = true },
    };
    
    for (test_cases) |tc| {
        const pattern = try allocator.dupeZ(u8, tc.pattern);
        defer allocator.free(pattern);
        
        const dir_z = try allocator.dupeZ(u8, test_dir);
        defer allocator.free(dir_z);
        
        const result = c.cuda_search_files(pattern.ptr, dir_z.ptr);
        try std.testing.expectEqual(tc.expected, result);
    }
}

// CPU baseline implementation for fair comparison
fn cpuSearchFiles(pattern: []const u8, directory: []const u8, allocator: std.mem.Allocator) !bool {
    var dir = try std.fs.cwd().openDir(directory, .{ .iterate = true });
    defer dir.close();
    
    var iterator = dir.iterate();
    while (try iterator.next()) |entry| {
        if (entry.kind != .file) continue;
        
        // Read file content
        const file = try dir.openFile(entry.name, .{});
        defer file.close();
        
        const content = try file.readToEndAlloc(allocator, 1024 * 1024);  // 1MB max
        defer allocator.free(content);
        
        // Search for pattern
        if (std.mem.indexOf(u8, content, pattern) != null) {
            return true;
        }
    }
    
    return false;
}

test "CPU vs GPU Search Benchmark" {
    const fs = std.fs;
    const allocator = std.testing.allocator;
    
    const test_dir = "benchmark_files";
    const num_files = 1000;
    const file_size = 10_000;  // 10KB per file
    const key = "SpecialBenchmarkKey";
    const key_index = 523;
    
    // Create test directory
    try fs.cwd().makeDir(test_dir);
    defer fs.cwd().deleteTree(test_dir) catch {};
    
    std.debug.print("\n=== Benchmark Setup ===\n", .{});
    std.debug.print("Files: {d}\n", .{num_files});
    std.debug.print("File size: ~{d} bytes\n", .{file_size});
    std.debug.print("Key location: file_{d}.txt\n", .{key_index});
    
    // Generate realistic file content
    var prng = std.Random.DefaultPrng.init(@intCast(std.time.timestamp()));
    const random = prng.random();
    
    const setup_start = std.time.nanoTimestamp();
    
    for (0..num_files) |i| {
        const file_name = try std.fmt.allocPrint(allocator, "{s}/file_{d}.txt", .{ test_dir, i });
        defer allocator.free(file_name);
        
        const f = try fs.cwd().createFile(file_name, .{});
        defer f.close();
        
        // Generate random content
        var content = try allocator.alloc(u8, file_size);
        defer allocator.free(content);
        
        for (content) |*byte| {
            byte.* = random.intRangeAtMost(u8, 'a', 'z');
        }
        
        // Insert key at specific location
        if (i == key_index) {
            const key_pos = file_size / 2;  // Middle of file
            @memcpy(content[key_pos..][0..key.len], key);
        }
        
        try f.writeAll(content);
    }
    
    const setup_end = std.time.nanoTimestamp();
    const setup_time: f64 = @floatFromInt(setup_end - setup_start);
    std.debug.print("Setup time: {d:.2}ms\n\n", .{setup_time / 1_000_000.0});
    
    // Test patterns
    const test_patterns = [_]struct {
        name: []const u8,
        pattern: []const u8,
        expected: bool,
    }{
        .{ .name = "Found (middle)", .pattern = key, .expected = true },
        .{ .name = "Not found", .pattern = "NonexistentPattern", .expected = false },
        .{ .name = "Short pattern", .pattern = "abc", .expected = true },  // Random chars likely exist
    };
    
    std.debug.print("=== Benchmark Results ===\n", .{});
    
    for (test_patterns) |tc| {
        std.debug.print("\nPattern: '{s}' ({s})\n", .{ tc.pattern, tc.name });
        
        // Warm-up run (important for GPU)
        {
            const pattern_z = try allocator.dupeZ(u8, tc.pattern);
            defer allocator.free(pattern_z);
            const dir_z = try allocator.dupeZ(u8, test_dir);
            defer allocator.free(dir_z);
            _ = c.cuda_search_files(pattern_z.ptr, dir_z.ptr);
        }
        
        // CPU benchmark
        const cpu_iterations = 5;
        var cpu_total: i128 = 0;
        
        for (0..cpu_iterations) |_| {
            const cpu_start = std.time.nanoTimestamp();
            const cpu_result = try cpuSearchFiles(tc.pattern, test_dir, allocator);
            const cpu_end = std.time.nanoTimestamp();
            
            try std.testing.expectEqual(tc.expected, cpu_result);
            cpu_total += cpu_end - cpu_start;
        }
        
        const cpu_avg: f64 = @floatFromInt(@divTrunc(cpu_total, cpu_iterations));
        const cpu_ms = cpu_avg / 1_000_000.0;
        
        // GPU benchmark
        const gpu_iterations = 5;
        var gpu_total: i128 = 0;
        
        for (0..gpu_iterations) |_| {
            const pattern_z = try allocator.dupeZ(u8, tc.pattern);
            defer allocator.free(pattern_z);
            const dir_z = try allocator.dupeZ(u8, test_dir);
            defer allocator.free(dir_z);
            
            const gpu_start = std.time.nanoTimestamp();
            const gpu_result = c.cuda_search_files(pattern_z.ptr, dir_z.ptr);
            const gpu_end = std.time.nanoTimestamp();
            
            try std.testing.expectEqual(tc.expected, gpu_result);
            gpu_total += gpu_end - gpu_start;
        }
        
        const gpu_avg: f64 = @floatFromInt(@divTrunc(gpu_total, gpu_iterations));
        const gpu_ms = gpu_avg / 1_000_000.0;
        
        const speedup = cpu_ms / gpu_ms;
        
        std.debug.print("  CPU:    {d:.2}ms (avg of {d} runs)\n", .{ cpu_ms, cpu_iterations });
        std.debug.print("  GPU:    {d:.2}ms (avg of {d} runs)\n", .{ gpu_ms, gpu_iterations });
        std.debug.print("  Speedup: {d:.2}x\n", .{speedup});
    }
}

test "Scalability Benchmark" {
    const allocator = std.testing.allocator;
    
    const test_configs = [_]struct {
        num_files: usize,
        file_size: usize,
    }{
        .{ .num_files = 100, .file_size = 1_000 },      // 100KB total
        .{ .num_files = 500, .file_size = 5_000 },      // 2.5MB total
        .{ .num_files = 1000, .file_size = 10_000 },    // 10MB total
        .{ .num_files = 2000, .file_size = 20_000 },    // 40MB total
    };
    
    std.debug.print("\n=== Scalability Test ===\n", .{});
    
    for (test_configs) |config| {
        const test_dir = try std.fmt.allocPrint(allocator, "scale_{d}_{d}", .{ config.num_files, config.file_size });
        defer allocator.free(test_dir);
        
        try std.fs.cwd().makeDir(test_dir);
        defer std.fs.cwd().deleteTree(test_dir) catch {};
        
        // Create files
        for (0..config.num_files) |i| {
            const file_name = try std.fmt.allocPrint(allocator, "{s}/file_{d}.txt", .{ test_dir, i });
            defer allocator.free(file_name);
            
            const f = try std.fs.cwd().createFile(file_name, .{});
            defer f.close();
            
            const content = try allocator.alloc(u8, config.file_size);
            defer allocator.free(content);
            @memset(content, 'x');
            
            try f.writeAll(content);
        }
        
        const pattern = "NonexistentPattern";
        const pattern_z = try allocator.dupeZ(u8, pattern);
        defer allocator.free(pattern_z);
        const dir_z = try allocator.dupeZ(u8, test_dir);
        defer allocator.free(dir_z);
        
        // GPU test
        const gpu_start = std.time.nanoTimestamp();
        _ = c.cuda_search_files(pattern_z.ptr, dir_z.ptr);
        const gpu_end = std.time.nanoTimestamp();
        const gpu_time: f64 = @floatFromInt(gpu_end - gpu_start);
        
        const total_mb: f64 = @floatFromInt(config.num_files * config.file_size);
        const throughput = (total_mb / 1_000_000.0) / (gpu_time / 1_000_000_000.0);
        
        std.debug.print("\nFiles: {d}, Size: {d}B, Total: {d:.2}MB\n", .{ config.num_files, config.file_size, total_mb / 1_000_000.0 });
        std.debug.print("  GPU Time: {d:.2}ms\n", .{gpu_time / 1_000_000.0});
        std.debug.print("  Throughput: {d:.2} MB/s\n", .{throughput});
    }
}
