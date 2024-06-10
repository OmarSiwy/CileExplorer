const std = @import("std");

const C = @cImport({
    @cInclude("GraphicsUser.h");
});

test "Rabin-Karp Match Test" {
    // Allocator setup
    const allocator = std.heap.page_allocator;

    // Test data
    const pattern = "findme";
    const haystack = [_][]const u8{
        "this is a findme test",
        "no match here",
        "another findme instance",
    };
    const num_strings: c_int = @intCast(haystack.len); // Casting length to c_int if necessary
    var match_count: c_int = 0;

    // Allocate space for C pointer array
    const c_haystack = try allocator.alloc([*]const u8, haystack.len);
    defer allocator.free(c_haystack);

    // Convert Zig strings to C pointers
    for (haystack) |item, i| {
        c_haystack[i] = item.ptr;
    }

    // Call the C function
    const matches = C.StartMatch(pattern.ptr, c_haystack.ptr, num_strings, &match_count);

    // Check results and output matches
    try std.testing.expect(match_count > 0);
    if (match_count > 0) {
        const ptr = @ptrCast([*][*]const u8, matches);
        const positive_match_count: usize = @intCast(match_count);
        for (positive_match_count) |i| {
            std.debug.print("Match found: {}\n", .{ptr[i]});
        }
    }
}
