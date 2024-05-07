const std = @import("std");
const info = std.log.info;

const c = @cImport({
    @cDefine("CL_TARGET_OPENCL_VERSION", "110");
    @cInclude("CL/cl.h");
});

const CLError = error{
    GetPlatformsFailed,
    GetPlatformInfoFailed,
    NoPlatformsFound,
    GetDevicesFailed,
    GetDeviceInfoFailed,
    NoDevicesFound,
    CreateContextFailed,
    CreateCommandQueueFailed,
    CreateProgramFailed,
    BuildProgramFailed,
    CreateKernelFailed,
    SetKernelArgFailed,
    EnqueueNDRangeKernel,
    CreateBufferFailed,
    EnqueueWriteBufferFailed,
    EnqueueReadBufferFailed,
};

fn get_cl_device() CLError!c.cl_device_id {
    var platform_ids: [16]c.cl_platform_id = undefined;
    var platform_count: c.cl_uint = undefined;
    if (c.clGetPlatformIDs(platform_ids.len, &platform_ids, &platform_count) != c.CL_SUCCESS) {
        return CLError.GetPlatformsFailed;
    }
    const platform_count_int: u32 = @intCast(platform_count);
    info("{} cl platform(s) found:", .{platform_count_int});

    for (platform_ids[0..platform_count], 0..platform_count) |id, i| {
        var name: [1024]u8 = undefined;
        var name_len: usize = undefined;
        if (c.clGetPlatformInfo(id, c.CL_PLATFORM_NAME, name.len, &name, &name_len) != c.CL_SUCCESS) {
            return CLError.GetPlatformInfoFailed;
        }
        info("  platform {}: {s}", .{ i, name[0..name_len] });
    }

    if (platform_count == 0) {
        return CLError.NoPlatformsFound;
    }

    info("choosing platform 0...", .{});

    var device_ids: [16]c.cl_device_id = undefined;
    var device_count: c.cl_uint = undefined;
    if (c.clGetDeviceIDs(platform_ids[0], c.CL_DEVICE_TYPE_ALL, device_ids.len, &device_ids, &device_count) != c.CL_SUCCESS) {
        return CLError.GetDevicesFailed;
    }
    const device_count_int: u32 = @intCast(device_count);
    info("{} cl device(s) found on platform 0:", .{device_count_int});

    for (device_ids[0..device_count], 0..device_count) |id, i| {
        var name: [1024]u8 = undefined;
        var name_len: usize = undefined;
        if (c.clGetDeviceInfo(id, c.CL_DEVICE_NAME, name.len, &name, &name_len) != c.CL_SUCCESS) {
            return CLError.GetDeviceInfoFailed;
        }
        info("  device {}: {s}", .{ i, name[0..name_len] });
    }

    if (device_count == 0) {
        return CLError.NoDevicesFound;
    }

    info("choosing device 0...", .{});

    return device_ids[0];
}
