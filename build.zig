const std = @import("std");
const ArrayList = std.ArrayList;

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    var targets = std.ArrayList(*std.Build.CompileStep).init(b.allocator);

    const cflags = .{
        "-std=c99",
        "-gen-cdb-fragment-path",
        "compile_commands",
        "-Wall",
        "-W",
        "-g",
        "-O2",
        "-fstack-protector-strong",
        "-D_FORTIFY_SOURCE=2",
        "-Wstrict-prototypes",
        "-Wwrite-strings",
        "-Wno-missing-field-initializers",
        "-fno-omit-frame-pointer",
    };

    const cLib = b.addStaticLibrary(.{
        .name = "cLib",
        .target = target,
        .optimize = optimize,
    });
    cLib.linkLibC();
    cLib.addIncludePath(.{
        .path = "include",
    });
    cLib.addCSourceFiles(&.{
        "src/helper/Search.c",
        "src/helper/GraphicsUser.c", // Add other C files as needed
    }, &cflags);
    cLib.force_pic = true;
    const gtkOptions = .{
        .needed = true,
    };
    cLib.addIncludePath(.{
        .path = "OpenCL-Headers",
    });
    cLib.linkSystemLibrary2("gtk+-3.0", gtkOptions);

    switch (target.getOs().tag) {
        .windows => {
            return; // Windows is not suported
        },
        .macos => {
            cLib.linkFramework("OpenCL");
        },
        else => {
            cLib.linkSystemLibrary("OpenCL");
        },
    }
    targets.append(cLib) catch @panic("OOM");

    // Create the Executable
    const Explorer = b.addExecutable(.{
        .name = "CileExplorer",
        .root_source_file = .{ .path = "src/main.c" }, // Main File
        .optimize = optimize,
        .target = target,
    });

    Explorer.linkLibC();
    Explorer.addIncludePath(.{
        .path = "include",
    });
    Explorer.linkLibrary(cLib);
    targets.append(Explorer) catch @panic("OOM");

    // Add a Convenience Run Step to Run Application, This is not needed
    b.installArtifact(Explorer);
    const run_step = b.step("run", "Run the Application");
    const run_exe = b.addRunArtifact(Explorer);
    run_step.dependOn(&run_exe.step);

    // Add a Convenience Step to Test Application, This is not needed
    const test_step = b.step("test", "Run unit tests");
    const unit_tests = b.addTest(.{
        .root_source_file = .{ .path = "tests/c_test.zig" },
        .target = target,
    });

    unit_tests.linkLibC();

    unit_tests.addIncludePath(.{
        .path = "include",
    });
    unit_tests.linkLibrary(cLib);

    const run_unit_tests = b.addRunArtifact(unit_tests);
    test_step.dependOn(&run_unit_tests.step);

    const write_compile_commands_step = b.step("writecc", "Write compile_commands.json");
    write_compile_commands_step.makeFn = maybeCreateCompileCommands;
    write_compile_commands_step.dependOn(&Explorer.step);
}

pub fn maybeCreateCompileCommands(_: *const std.build.Builder.Step, _: *std.Progress.Node) !void {
    const dir = try std.fs.cwd().openIterableDir("compile_commands", .{});
    var iterator = dir.iterate();

    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    const allocator = gpa.allocator();

    defer {
        if (.ok != gpa.deinit()) {
            @panic("OFSMC!  Some memory leaked!");
        }
    }

    var my_file = std.fs.cwd().createFile("compile_commands.json", .{}) catch |err| {
        std.debug.print("\nError: {}\n", .{err});
        return;
    };
    defer my_file.close();

    var comma: bool = false;

    var wrote_bytes = my_file.write("[\n") catch |err| {
        std.debug.print("\nError writing: {}\n", .{err});
        return err;
    };

    while (try iterator.next()) |path| {
        var relative_name = std.ArrayList(u8).init(allocator);
        defer relative_name.deinit();

        relative_name.appendSlice("compile_commands/") catch |err| {
            std.debug.print("\nError building string: {}\n", .{err});
            return err;
        };

        relative_name.appendSlice(path.name) catch |err| {
            std.debug.print("\nError building string: {}\n", .{err});
            return err;
        };

        var json_file = try std.fs.cwd().openFile(relative_name.items, .{});
        defer json_file.close();

        var cc_json_string: [1024 * 1024]u8 = undefined;
        const bytes_read = try json_file.read(&cc_json_string);

        var last_byte: usize = bytes_read;
        if (cc_json_string[(bytes_read - 2)] == ',') {
            last_byte -= 2;
        }

        const stream_bytes = cc_json_string[0..last_byte];

        if (comma) {
            wrote_bytes = my_file.write(",") catch |err| {
                std.debug.print("\nError writing: {}\n", .{err});
                return err;
            };
        }

        wrote_bytes = my_file.write(stream_bytes) catch |err| {
            std.debug.print("\nError writing: {}\n", .{err});
            return err;
        };

        comma = true;
    }

    wrote_bytes = my_file.write("\n]\n") catch |err| {
        std.debug.print("\nError: {}\n", .{err});
        return err;
    };
}
