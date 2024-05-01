const std = @import("std");

const C = @cImport({
    @cInclude("prompt.h");
});

test "C_Exports Test" {
    const result = C.add(1, 2); // Call C function
    try std.testing.expect(result == 3); // Direct comparison to literal
}
