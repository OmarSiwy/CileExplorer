const std = @import("std");
const info = std.log.info;

const c = @cImport({
    @cDefine("CL_TARGET_OPENCL_VERSION", "110");
    @cInclude("CL/cl.h");
});

const OpenCL = @import("OpenCL.zig");

// Function to be run on the OpenCL device
const program_src =
    \\__kernel void square_array(__global int* input_array, __global int* output_array) {
    \\    int i = get_global_id(0);
    \\    int value = input_array[i];
    \\    output_array[i] = value * value;
    \\}
;

fn run_test(device: c.cl_device_id) OpenCL.CLError!void {
    info("** running test **", .{});

    var ctx = c.clCreateContext(null, 1, &device, null, null, null); // future: last arg is error code
    if (ctx == null) {
        return OpenCL.CLError.CreateContextFailed;
    }
    defer _ = c.clReleaseContext(ctx);

    var command_queue = c.clCreateCommandQueue(ctx, device, 0, null); // future: last arg is error code
    if (command_queue == null) {
        return OpenCL.CLError.CreateCommandQueueFailed;
    }
    defer {
        _ = c.clFlush(command_queue);
        _ = c.clFinish(command_queue);
        _ = c.clReleaseCommandQueue(command_queue);
    }

    var program_src_c: [*c]const u8 = program_src;
    var program = c.clCreateProgramWithSource(ctx, 1, &program_src_c, null, null); // future: last arg is error code
    if (program == null) {
        return OpenCL.CLError.CreateProgramFailed;
    }
    defer _ = c.clReleaseProgram(program);

    if (c.clBuildProgram(program, 1, &device, null, null, null) != c.CL_SUCCESS) {
        return OpenCL.CLError.BuildProgramFailed;
    }

    var kernel = c.clCreateKernel(program, "square_array", null);
    if (kernel == null) {
        return OpenCL.CLError.CreateKernelFailed;
    }
    defer _ = c.clReleaseKernel(kernel);

    // Create buffers
    var input_array = init: {
        var init_value: [1024]i32 = undefined;
        for (init_value, 0..1024) |*pt, i| {
            pt.* = @intCast(i);
        }
        break :init init_value;
    };

    var input_buffer = c.clCreateBuffer(ctx, c.CL_MEM_READ_ONLY, input_array.len * @sizeOf(i32), null, null);
    if (input_buffer == null) {
        return OpenCL.CLError.CreateBufferFailed;
    }
    defer _ = c.clReleaseMemObject(input_buffer);

    var output_buffer = c.clCreateBuffer(ctx, c.CL_MEM_WRITE_ONLY, input_array.len * @sizeOf(i32), null, null);
    if (output_buffer == null) {
        return OpenCL.CLError.CreateBufferFailed;
    }
    defer _ = c.clReleaseMemObject(output_buffer);

    // Fill input buffer
    if (c.clEnqueueWriteBuffer(command_queue, input_buffer, c.CL_TRUE, 0, input_array.len * @sizeOf(i32), &input_array, 0, null, null) != c.CL_SUCCESS) {
        return OpenCL.CLError.EnqueueWriteBufferFailed;
    }

    // Execute kernel
    if (c.clSetKernelArg(kernel, 0, @sizeOf(c.cl_mem), &input_buffer) != c.CL_SUCCESS) {
        return OpenCL.CLError.SetKernelArgFailed;
    }
    if (c.clSetKernelArg(kernel, 1, @sizeOf(c.cl_mem), &output_buffer) != c.CL_SUCCESS) {
        return OpenCL.CLError.SetKernelArgFailed;
    }

    var global_item_size: usize = input_array.len;
    var local_item_size: usize = 64;
    if (c.clEnqueueNDRangeKernel(command_queue, kernel, 1, null, &global_item_size, &local_item_size, 0, null, null) != c.CL_SUCCESS) {
        return OpenCL.CLError.EnqueueNDRangeKernel;
    }

    var output_array: [1024]i32 = undefined;
    if (c.clEnqueueReadBuffer(command_queue, output_buffer, c.CL_TRUE, 0, output_array.len * @sizeOf(i32), &output_array, 0, null, null) != c.CL_SUCCESS) {
        return OpenCL.CLError.EnqueueReadBufferFailed;
    }

    info("** done **", .{});

    info("** results **", .{});

    for (output_array, 0..1024) |val, i| {
        if (i % 100 == 0) {
            info("{} ^ 2 = {}", .{ i, val });
        }
    }

    info("** done, exiting **", .{});
}

pub const FuncPtr = fn (input: [*c]const u8) callconv(.C) [*c]const u8;
pub fn Search(input: [*c]const u8) callconv(.C) [*c]const u8 {
    return input;
}
