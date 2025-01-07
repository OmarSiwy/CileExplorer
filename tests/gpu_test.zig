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
        // Allocate null-terminated strings
        const pattern = try allocator.dupeZ(u8, tc.pattern);
        defer allocator.free(pattern);

        const dir_z = try allocator.dupeZ(u8, test_dir);
        defer allocator.free(dir_z);

        // Call CUDA search function
        const result = c.cuda_search_files(pattern.ptr, dir_z.ptr);
        try std.testing.expectEqual(tc.expected, result);
    }
}

test "CUDA Search Benchmark for File Name Matching" {
    const fs = std.fs;
    const allocator = std.testing.allocator;

    const test_dir = "test_files";
    const num_files = 1000; // Total number of files
    const key = "Benchmark"; // The special key
    const key_index = 523; // The index of the file containing the key

    // Create the test directory
    try fs.cwd().makeDir(test_dir);
    defer fs.cwd().deleteTree(test_dir) catch {};

    // Create test files
    for (0..num_files) |i| {
        const file_name = try std.fmt.allocPrint(allocator, "{s}/file_{d}.txt", .{ test_dir, i });
        defer allocator.free(file_name);

        const content = if (i == key_index) try std.fmt.allocPrint(allocator, "This is the special file containing the key: {s}\n", .{key}) else try std.fmt.allocPrint(allocator, "Dummy content for file number {d}\n", .{i});
        defer allocator.free(content);

        const f = try fs.cwd().createFile(file_name, .{});
        defer f.close();
        try f.writeAll(content);
    }

    std.debug.print("Created {d} files in directory '{s}', with key at index {d}.\n", .{ num_files, test_dir, key_index });

    // Define test cases
    const test_cases = [_]struct {
        pattern: []const u8,
        expected: bool,
    }{
        .{ .pattern = "Benchmark", .expected = true },
        .{ .pattern = "Nonexistent", .expected = false },
    };

    const start = std.time.nanoTimestamp();
    for (test_cases) |tc| {
        // Allocate null-terminated strings
        const pattern = try allocator.dupeZ(u8, tc.pattern);
        defer allocator.free(pattern);

        const dir_z = try allocator.dupeZ(u8, test_dir);
        defer allocator.free(dir_z);

        // Call CUDA search function
        const result = c.cuda_search_files(pattern.ptr, dir_z.ptr);
        try std.testing.expectEqual(tc.expected, result);
    }
    const end = std.time.nanoTimestamp();

    const timeinfloat: f64 = @floatFromInt(end - start);
    const elapsed_time = timeinfloat / 1_000_000.0;
    std.debug.print("Execution time for test cases: {:.2} ms\n", .{elapsed_time});
}
