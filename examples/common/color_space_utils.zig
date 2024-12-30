/// Convert a fraction i / max to a rainbow color in YUV format.
///
/// For example, `rainbow(0, 100)` returns the first color of the rainbow.
/// And `rainbow(100, 100)` returns the last color of the rainbow.
pub fn rainbow(i: usize, max: usize) struct { y: u8, u: u8, v: u8 } {
    const p = @as(f32, @floatFromInt(i)) / @as(f32, @floatFromInt(max));
    const hue = p * 360.0;
    const rgb = hsv2rgb(hue, 100.0, 100.0);
    const yuv = rgb2yuv(rgb[0], rgb[1], rgb[2]);
    return .{
        .y = @intFromFloat(yuv[0]),
        .u = @intFromFloat(yuv[1]),
        .v = @intFromFloat(yuv[2]),
    };
}

pub fn hsv2rgb(h: f32, s: f32, v: f32) [3]f32 {
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

pub fn rgb2yuv(r: f32, g: f32, b: f32) [3]f32 {
    return .{
        0.257 * r + 0.504 * g + 0.098 * b + 16.0,
        -0.148 * r - 0.291 * g + 0.439 * b + 128.0,
        0.439 * r - 0.368 * g - 0.071 * b + 128.0,
    };
}
