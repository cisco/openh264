const std = @import("std");
const openh264 = @import("openh264");

// TODO: Useful Zig wrapper layer.

const FORMAT = openh264.EVideoFormatType.rgb;
const NUM_FRAMES = 256;

pub fn main() !void {
    const file = try std.fs.cwd().createFile("rainbow.h264", .{});
    defer file.close();

    var rc: c_int = undefined;

    var encoder: ?*openh264.ISVCEncoder = null;
    rc = openh264.WelsCreateSVCEncoder(&encoder);
    std.debug.assert(rc == 0);
    std.debug.assert(encoder != null);
    defer if (encoder != null) openh264.WelsDestroySVCEncoder(encoder);

    const parameters = openh264.SEncParamBase{
        .iUsageType = .screen_content_non_real_time,
        .iPicWidth = 1920,
        .iPicHeight = 1080,
        .fMaxFrameRate = 30.0,
        .iTargetBitrate = 5000000,
        .iRCMode = .quality_mode,
    };
    rc = encoder.?.*.?.*.Initialize.?(encoder, &parameters);
    std.debug.assert(rc == 0);
    defer _ = encoder.?.*.?.*.Uninitialize.?(encoder);

    rc = encoder.?.*.?.*.SetOption.?(encoder, .dataformat, &FORMAT);
    std.debug.assert(rc == 0);

    var info = std.mem.zeroes(openh264.SFrameBSInfo);
    var picture_data = .{
        .r = std.mem.zeroes([1920 * 1080]u8),
        .g = std.mem.zeroes([1920 * 1080]u8),
        .b = std.mem.zeroes([1920 * 1080]u8),
    };
    var picture = openh264.SSourcePicture{
        .iColorFormat = .rgb,
        .iStride = .{
            parameters.iPicWidth,
            parameters.iPicWidth,
            parameters.iPicWidth,
            0,
        },
        .iPicWidth = parameters.iPicWidth,
        .iPicHeight = parameters.iPicHeight,
        .pData = .{
            &picture_data.r,
            &picture_data.g,
            &picture_data.b,
            null,
        },
        .uiTimeStamp = 0,
    };

    for (0..NUM_FRAMES) |i| {
        const p = @as(f32, @floatFromInt(i)) / @as(f32, @floatFromInt(NUM_FRAMES));
        const hue = p * 360.0;
        const rgb = hsv2rgb(hue, 100.0, 100.0);
        @memset(&picture_data.r, rgb[0]);
        @memset(&picture_data.g, rgb[1]);
        @memset(&picture_data.b, rgb[2]);
        rc = encoder.?.*.?.*.EncodeFrame.?(encoder, &picture, &info);
        std.debug.assert(rc == 0);
        if (info.eFrameType != .skip) {
            for (0..@intCast(info.iLayerNum)) |layer_index| {
                const layer = info.sLayerInfo[layer_index];
                var nal_offset: usize = 0;
                for (0..@intCast(layer.iNalCount)) |nal_index| {
                    const nal_len: usize = @intCast(layer.pNalLengthInByte[nal_index]);
                    defer nal_offset += nal_len;
                    const nal_data = layer.pBsBuf[nal_offset..nal_len];
                    try file.writeAll(nal_data);
                }
            }
        }
    }
}

fn hsv2rgb(h: f32, s: f32, v: f32) [3]u8 {
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
        @as(u8, @intFromFloat(((rgb[0] + m) * 255.0))),
        @as(u8, @intFromFloat(((rgb[1] + m) * 255.0))),
        @as(u8, @intFromFloat(((rgb[2] + m) * 255.0))),
    };
}
