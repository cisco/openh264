/// This example shows how to use the low-level bidings.
const std = @import("std");
const openh264_bindings = @import("openh264_bindings");

const openh264_trace_level: openh264_bindings.WelsLogLevel = .debug;
const rainbow_num_frames = 256;
const format = openh264_bindings.EVideoFormatType.i420; // This is the only supported color format for encoding.

pub fn main() !void {
    var general_purpose_allocator = std.heap.GeneralPurposeAllocator(.{}){};
    defer std.debug.assert(general_purpose_allocator.deinit() == .ok);

    const allocator = general_purpose_allocator.allocator();

    const file = try std.fs.cwd().createFile("rainbow.264", .{});
    defer file.close();

    var rc: c_int = undefined;

    var encoder: ?*openh264_bindings.ISVCEncoder = null;
    rc = openh264_bindings.WelsCreateSVCEncoder(&encoder);
    std.debug.assert(rc == 0);
    std.debug.assert(encoder != null);
    defer openh264_bindings.WelsDestroySVCEncoder(encoder);

    const parameters = openh264_bindings.SEncParamBase{
        .iUsageType = .screen_content_real_time,
        .iPicWidth = 1920,
        .iPicHeight = 1080,
        .fMaxFrameRate = 30.0,
        .iTargetBitrate = 5000000,
        .iRCMode = .quality_mode,
    };
    rc = encoder.?.*.?.*.Initialize.?(encoder, &parameters);
    std.debug.assert(rc == 0);
    defer _ = encoder.?.*.?.*.Uninitialize.?(encoder);

    rc = encoder.?.*.?.*.SetOption.?(encoder, .trace_level, &openh264_trace_level);
    std.debug.assert(rc == 0);

    rc = encoder.?.*.?.*.SetOption.?(encoder, .dataformat, &format);
    std.debug.assert(rc == 0);

    var info = std.mem.zeroes(openh264_bindings.SFrameBSInfo);
    const picture_data = .{
        .y = try allocator.alloc(u8, parameters.iPicWidth * parameters.iPicHeight),
        .u = try allocator.alloc(u8, ((parameters.iPicWidth / 2) * (parameters.iPicHeight / 2))),
        .v = try allocator.alloc(u8, ((parameters.iPicWidth / 2) * (parameters.iPicHeight / 2))),
    };
    std.debug.assert((picture_data.y.len + picture_data.u.len + picture_data.v.len) == (parameters.iPicWidth * parameters.iPicHeight) * 3 / 2);
    defer {
        allocator.free(picture_data.y);
        allocator.free(picture_data.u);
        allocator.free(picture_data.v);
    }
    var picture = openh264_bindings.SSourcePicture{
        .iColorFormat = format,
        .iStride = .{
            parameters.iPicWidth,
            parameters.iPicWidth / 2,
            parameters.iPicWidth / 2,
            0,
        },
        .iPicWidth = parameters.iPicWidth,
        .iPicHeight = parameters.iPicHeight,
        .pData = .{
            picture_data.y.ptr,
            picture_data.u.ptr,
            picture_data.v.ptr,
            null,
        },
        .uiTimeStamp = 0,
    };

    for (0..rainbow_num_frames) |i| {
        const p = @as(f32, @floatFromInt(i)) / @as(f32, @floatFromInt(rainbow_num_frames));
        const hue = p * 360.0;
        const rgb = hsv2rgb(hue, 100.0, 100.0);
        const yuv = rgb2yuv(rgb[0], rgb[1], rgb[2]);
        @memset(picture_data.y, @intFromFloat(yuv[0]));
        @memset(picture_data.u, @intFromFloat(yuv[1]));
        @memset(picture_data.v, @intFromFloat(yuv[2]));
        rc = encoder.?.*.?.*.EncodeFrame.?(encoder, &picture, &info);
        std.debug.assert(rc == 0);
        if (info.eFrameType != .skip) {
            for (0..@intCast(info.iLayerNum)) |layer_index| {
                const layer = info.sLayerInfo[layer_index];
                var nal_offset: usize = 0;
                for (0..@intCast(layer.iNalCount)) |nal_index| {
                    const nal_len: usize = @intCast(layer.pNalLengthInByte[nal_index]);
                    defer nal_offset += nal_len;
                    const nal_data = layer.pBsBuf[nal_offset .. nal_offset + nal_len];
                    try file.writeAll(nal_data);
                }
            }
        }
    }
}

fn hsv2rgb(h: f32, s: f32, v: f32) [3]f32 {
    const c = (s / 100.0) * (v / 100.0);
    const x = c * (1.0 - @abs(@mod((h / 60.0), 2.0) - 1.0));
    const m = (v / 100.0) - c;
    const rgb = if (h < 60.0)
        .{ c, x, 0.0 }
    else if (h < 120.0)
        .{ x, c, 0.0 }
    else if (h < 180.0)
        .{ 0.0, c, x }
    else if (h < 240.0)
        .{ 0.0, x, c }
    else if (h < 300.0)
        .{ x, 0.0, c }
    else if (h < 360.0)
        .{ c, 0.0, x }
    else
        .{ 0.0, 0.0, 0.0 };
    return .{
        (rgb[0] + m) * 255.0,
        (rgb[1] + m) * 255.0,
        (rgb[2] + m) * 255.0,
    };
}

fn rgb2yuv(r: f32, g: f32, b: f32) [3]f32 {
    return .{
        0.257 * r + 0.504 * g + 0.098 * b + 16.0,
        -0.148 * r - 0.291 * g + 0.439 * b + 128.0,
        0.439 * r - 0.368 * g - 0.071 * b + 128.0,
    };
}
