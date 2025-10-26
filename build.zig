const std = @import("std");
const builtin = @import("builtin");
const zcc = @import("CC.zig");

pub fn build(b: *std.Build) !void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // Compiler flags
    const cflags = &.{
        "-std=c99",    "-gen-cdb-fragment-path",   "compile_commands",    "-Wall",               "-W",              "-g",                              "-O2",
        "-ffast-math", "-fstack-protector-strong", "-D_FORTIFY_SOURCE=2", "-Wstrict-prototypes", "-Wwrite-strings", "-Wno-missing-field-initializers", "-fno-omit-frame-pointer",
    };
    const gtkflags = try getPkgConfigFlags(b);

    // Get CUDA paths from environment (set by Nix)
    const cuda_include = std.process.getEnvVarOwned(b.allocator, "NIX_CUDA_INCLUDE_PATH") catch blk: {
        std.debug.print("Warning: NIX_CUDA_INCLUDE_PATH not set, using fallback\n", .{});
        break :blk try b.allocator.dupe(u8, "/opt/cuda/include");
    };
    const cuda_lib = std.process.getEnvVarOwned(b.allocator, "NIX_CUDA_LIB_PATH") catch blk: {
        std.debug.print("Warning: NIX_CUDA_LIB_PATH not set, using fallback\n", .{});
        break :blk try b.allocator.dupe(u8, "/usr/local/cuda/lib64");
    };

    std.debug.print("Using CUDA include: {s}\n", .{cuda_include});
    std.debug.print("Using CUDA lib: {s}\n", .{cuda_lib});

    // CUDA cache setup
    const cuda_cache_dir = b.pathJoin(&.{ ".zig-cache", "cuda" });
    std.fs.cwd().makePath(cuda_cache_dir) catch |err| {
        std.debug.print("Failed to create CUDA cache directory: {}\n", .{err});
        return err;
    };

    // CUDA compilation step with include path
    const cuda_include_flag = try std.fmt.allocPrint(b.allocator, "-I{s}", .{cuda_include});
    
    const cuda_step = b.addSystemCommand(&[_][]const u8{
        "nvcc",
        "-c",
        "include/cuda/search_kernel.cu",
        "-o",
        ".zig-cache/cuda/search_kernel.o",
        cuda_include_flag,
        "-allow-unsupported-compiler",
        "--compiler-options",
        "-fPIC",
        "-O2",
        "--std=c++14",
        "-DTHRUST_HOST_SYSTEM=THRUST_HOST_SYSTEM_CPP",
        "-DTHRUST_DEVICE_SYSTEM=THRUST_DEVICE_SYSTEM_CUDA",
        "-DTHRUST_DISABLE_EXCEPTIONS",
        "-Xcompiler",
        "-fPIC",
    });

    // Create static library (without system libraries linked)
    const cLib = b.addStaticLibrary(.{
        .name = "cLib",
        .target = target,
        .optimize = optimize,
    });
    
    cLib.linkLibC();
    cLib.addIncludePath(.{ .cwd_relative = "include" });
    cLib.addIncludePath(.{ .cwd_relative = cuda_include });
    
    // Add GTK include paths (but don't link yet)
    for (gtkflags) |gtk| {
        if (std.mem.startsWith(u8, gtk, "-I")) {
            cLib.addIncludePath(.{ .cwd_relative = gtk[2..] });
        }
    }

    // Add CUDA object file to library
    cLib.step.dependOn(&cuda_step.step);
    cLib.addObjectFile(.{ .cwd_relative = ".zig-cache/cuda/search_kernel.o" });

    // Add source files to library
    cLib.addCSourceFiles(.{
        .flags = cflags,
        .files = &.{ "src/Search.c", "src/Pages/Sidebar.c", "src/Pages/MainPage.c", "src/Pages/Topbar.c" },
    });

    // Create the executable
    const Explorer = b.addExecutable(.{
        .name = "CileExplorer",
        .optimize = optimize,
        .target = target,
    });
    
    Explorer.addCSourceFile(.{
        .file = .{ .cwd_relative = "main.c" },
        .flags = cflags,
    });
    
    Explorer.linkLibC();
    Explorer.addIncludePath(.{ .cwd_relative = "include" });
    
    // Add GTK include paths
    for (gtkflags) |gtk| {
        if (std.mem.startsWith(u8, gtk, "-I")) {
            Explorer.addIncludePath(.{ .cwd_relative = gtk[2..] });
        }
    }

    // Link the static library
    Explorer.linkLibrary(cLib);
    
    // Link system libraries (GTK and CUDA) to executable
    Explorer.linkSystemLibrary("gtk+-3.0");
    
    // Add CUDA library path and link CUDA libraries
    var it = std.mem.splitScalar(u8, cuda_lib, ':');
    while (it.next()) |lib_path| {
        if (lib_path.len > 0) {
            Explorer.addLibraryPath(.{ .cwd_relative = lib_path });
        }
    }
    Explorer.linkSystemLibrary("cudart");
    Explorer.linkSystemLibrary("cuda");

    // Install artifact
    b.installArtifact(Explorer);

    // Run Step
    const run_step = b.step("run", "Run the Application");
    const run_exe = b.addRunArtifact(Explorer);
    run_step.dependOn(&run_exe.step);

    // Test Step
    const test_step = b.step("test", "Run unit tests");
    const unit_tests = b.addTest(.{
        .root_source_file = .{ .cwd_relative = "tests/main_test.zig" },
        .target = target,
    });
    
    unit_tests.linkLibC();
    unit_tests.addIncludePath(.{ .cwd_relative = "include" });
    
    for (gtkflags) |gtk| {
        if (std.mem.startsWith(u8, gtk, "-I")) {
            unit_tests.addIncludePath(.{ .cwd_relative = gtk[2..] });
        }
    }
    
    unit_tests.linkLibrary(cLib);
    unit_tests.linkSystemLibrary("gtk+-3.0");
    
    // Add CUDA to tests
    it = std.mem.splitScalar(u8, cuda_lib, ':');
    while (it.next()) |lib_path| {
        if (lib_path.len > 0) {
            unit_tests.addLibraryPath(.{ .cwd_relative = lib_path });
        }
    }
    unit_tests.linkSystemLibrary("cudart");
    unit_tests.linkSystemLibrary("cuda");

    const run_unit_tests = b.addRunArtifact(unit_tests);
    test_step.dependOn(&run_unit_tests.step);

    // Add compile commands generation
    AddCompileCommandStep(b, cLib);
}

fn AddCompileCommandStep(b: *std.Build, lib: *std.Build.Step.Compile) void {
    var targets_files = std.ArrayList(*std.Build.Step.Compile).init(b.allocator);
    defer targets_files.deinit();
    targets_files.append(lib) catch @panic("OOM");
    zcc.createStep(b, "cdb", targets_files.toOwnedSlice() catch @panic("OOM"));
}

fn getPkgConfigFlags(b: *std.Build) ![]const []const u8 {
    var args = std.ArrayList([]const u8).init(b.allocator);

    const essential_includes = [_][]const u8{
        "gtk-3.0", "glib-2.0", "cairo", "pango-1.0", "gdk-pixbuf-2.0", "atk-1.0", "harfbuzz",
    };

    const result = try std.process.Child.run(.{
        .allocator = b.allocator,
        .argv = &.{ "pkg-config", "--cflags", "gtk+-3.0" },
    });

    var it = std.mem.splitScalar(u8, std.mem.trim(u8, result.stdout, " \n"), ' ');
    while (it.next()) |flag| {
        if (flag.len == 0) continue;
        for (essential_includes) |essential| {
            if (std.mem.indexOf(u8, flag, essential) != null) {
                try args.append(flag);
                break;
            }
        }
    }

    return args.toOwnedSlice();
}
