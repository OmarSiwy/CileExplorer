const std = @import("std");
const c = @import("CImports.zig");
const Search = @import("FuzzySearch.zig");

pub fn main() !void {
    const funcPtr: Search.FuncPtr = Search.Search;
    _ = c.run(funcPtr);
}
