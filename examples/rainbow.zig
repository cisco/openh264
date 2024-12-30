//! This example shows how to use the high-level bindings.
//!
//! The high-level bindings are more Zig-friendly and work around some of the
//! idiosyncrasies of the OpenH264 API.

const std = @import("std");

const openh264 = @import("openh264");

const rainbow = @import("common/color_space_utils.zig").rainbow;

const rainbow_num_frames = 256;

pub fn main() !void {
    var general_purpose_allocator = std.heap.GeneralPurposeAllocator(.{}){};
    defer std.debug.assert(general_purpose_allocator.deinit() == .ok);

    const allocator = general_purpose_allocator.allocator();

    const file = try std.fs.cwd().createFile("rainbow.264", .{});
    defer file.close();
    const writer = file.writer();

    var encoder = try openh264.Encoder.init(allocator, .{
        .resolution = .{ .width = 1920, .height = 1080 },
    });
    defer encoder.deinit();

    var frame = try openh264.Frame.init(1920, 1080, allocator);
    defer frame.deinit(allocator);

    for (0..rainbow_num_frames) |i| {
        const r = rainbow(i, rainbow_num_frames);
        @memset(frame.y, r.y);
        @memset(frame.u, r.u);
        @memset(frame.v, r.v);
        try encoder.encode(&frame, writer);
    }
}
