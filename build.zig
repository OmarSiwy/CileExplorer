const std = @import("std");
const builtin = @import("builtin");

pub fn build(b: *std.Build) !void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // Compiler flags
    const cflags = &.{
        "-std=c99",    "-gen-cdb-fragment-path",   "compile_commands",    "-Wall",               "-W",              "-g",                              "-O2",
        "-ffast-math", "-fstack-protector-strong", "-D_FORTIFY_SOURCE=2", "-Wstrict-prototypes", "-Wwrite-strings", "-Wno-missing-field-initializers", "-fno-omit-frame-pointer",
    };
    const gtkflags = try getPkgConfigFlags(b);

    // CUDA cache setup
    const cuda_cache_dir = b.pathJoin(&.{ ".zig-cache", "cuda" });
    std.fs.cwd().makePath(cuda_cache_dir) catch |err| {
        std.debug.print("Failed to create CUDA cache directory: {}\n", .{err});
        return err;
    };

    // CUDA compilation step
    const cuda_step = b.addSystemCommand(&[_][]const u8{
        "nvcc",                                        "-c",                                               "include/cuda/search_kernel.cu", "-o",         ".zig-cache/cuda/search_kernel.o",
        "-allow-unsupported-compiler",                 "--compiler-options",                               "-fPIC",                         "-O2",        "--std=c++14",
        "-DTHRUST_HOST_SYSTEM=THRUST_HOST_SYSTEM_CPP", "-DTHRUST_DEVICE_SYSTEM=THRUST_DEVICE_SYSTEM_CUDA", "-DTHRUST_DISABLE_EXCEPTIONS",   "-Xcompiler", "-fPIC",
    });

    // Create static library
    const cLib = b.addStaticLibrary(.{
        .name = "cLib",
        .target = target,
        .optimize = optimize,
    });
    try addCommonIncludes(cLib, gtkflags);

    // Link CUDA to library
    linkCuda(cLib, cuda_step);

    // Add source files to library
    cLib.addCSourceFiles(.{
        .flags = cflags,
        .files = &.{ "src/helper/Search.c", "src/Layouts/ExplorerLayout.c", "src/Pages/Sidebar.c", "src/Pages/MainPage.c", "src/Pages/Topbar.c" },
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
    try addCommonIncludes(Explorer, gtkflags);

    // Link library and CUDA to executable
    Explorer.linkLibrary(cLib);
    linkCuda(Explorer, null);

    // Add convenience steps
    // Run Step
    b.installArtifact(Explorer);
    const run_step = b.step("run", "Run the Application");
    const run_exe = b.addRunArtifact(Explorer);
    run_step.dependOn(&run_exe.step);

    // Test Step
    const test_step = b.step("test", "Run unit tests");
    const unit_tests = b.addTest(.{
        .root_source_file = .{ .cwd_relative = "tests/main_test.zig" },
        .target = target,
    });
    try addCommonIncludes(unit_tests, gtkflags);
    unit_tests.linkLibrary(cLib);
    linkCuda(unit_tests, null);

    const run_unit_tests = b.addRunArtifact(unit_tests);
    test_step.dependOn(&run_unit_tests.step);

    // Add compile commands generation
    AddCompileCommandStep(b, cLib);
}

// Helper functions
fn linkCuda(step: *std.Build.Step.Compile, cuda_step: ?*std.Build.Step.Run) void {
    if (cuda_step) |cs| { // Unwrap the optional
        step.step.dependOn(&cs.step);
        step.addObjectFile(.{ .cwd_relative = ".zig-cache/cuda/search_kernel.o" });
    }
    step.addIncludePath(.{ .cwd_relative = "/opt/cuda/include" });
    step.addLibraryPath(.{ .cwd_relative = "/usr/local/cuda/lib64" });
    step.linkSystemLibrary("cudart");
    step.linkSystemLibrary("cuda");
}

fn AddCompileCommandStep(b: *std.Build, lib: *std.Build.Step.Compile) void {
    const zcc = @import("Compile_Commands.zig");
    var targets_files = std.ArrayList(*std.Build.Step.Compile).init(b.allocator);
    defer targets_files.deinit();
    targets_files.append(lib) catch @panic("OOM");
    zcc.createStep(b, "cdb", targets_files.toOwnedSlice() catch @panic("OOM"));
}

fn getPkgConfigFlags(b: *std.Build) ![]const []const u8 {
    var args = std.ArrayList([]const u8).init(b.allocator);

    const essential_includes = [_][]const u8{
        "gtk-3.0", "glib-2.0", "cairo", "pango-1.0", "gdk-pixbuf-2.0", "atk-1.0",
    };

    const result = try std.process.Child.run(.{
        .allocator = b.allocator,
        .argv = &.{ "pkg-config", "--cflags", "gtk+-3.0" },
    });

    // gtk
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

fn addCommonIncludes(compile_step: *std.Build.Step.Compile, gtk_flags: []const []const u8) !void {
    compile_step.linkLibC();

    // Add project-specific includes
    compile_step.addIncludePath(.{ .cwd_relative = "include" });

    // Add GTK and related library include paths from pkg-config flags
    for (gtk_flags) |gtk| {
        if (std.mem.startsWith(u8, gtk, "-I")) {
            compile_step.addIncludePath(.{ .cwd_relative = gtk[2..] });
        }
    }

    // Add linking for GTK and GDK Pixbuf libraries
    switch (builtin.os.tag) {
        .macos => {
            std.debug.print("Mac\n\n", .{});
            compile_step.linkFramework("gtk+-3.0");
        },
        else => {
            std.debug.print("Windows/Linux\n\n", .{});
            compile_step.linkSystemLibrary("gtk+-3.0");
        },
    }
}
