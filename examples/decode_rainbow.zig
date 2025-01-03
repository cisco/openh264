//! Example to show how to decode a file.

const std = @import("std");

const openh264 = @import("openh264");

pub fn main() !void {
    var general_purpose_allocator = std.heap.GeneralPurposeAllocator(.{}){};
    defer std.debug.assert(general_purpose_allocator.deinit() == .ok);

    const allocator = general_purpose_allocator.allocator();

    var decoder = try openh264.Decoder.init(allocator, .{});
    defer decoder.deinit();

    const file = try std.fs.cwd().openFile("rainbow.264", .{});
    defer file.close();

    var buffer = try allocator.alloc(u8, 4096);
    defer allocator.free(buffer);

    var nal = std.ArrayList(u8).init(allocator);
    defer nal.deinit();

    while (true) {
        const len = try file.reader().readAll(buffer);

        var last_nal: usize = 0;

        const len_range = @max(len, 4) - 4;
        for (0..len_range) |index| {
            if (std.mem.eql(u8, buffer[index .. index + 4], &.{ 0, 0, 0, 1 })) {
                if (index - last_nal > 0) {
                    nal.appendSlice(buffer[last_nal..index]) catch @panic("oom");
                }
                if (nal.items.len > 0) {
                    if (try decoder.decode(nal.items)) |frame| {
                        handle_frame(&frame);
                    }
                    nal.clearRetainingCapacity();
                    last_nal = index;
                }
            }
        }

        if (last_nal < len) {
            nal.appendSlice(buffer[last_nal..len]) catch @panic("oom");
        }

        if (len < buffer.len) break;
    }

    if (try decoder.decode(nal.items)) |frame| {
        handle_frame(&frame);
    }

    try decoder.set_end_of_stream(true);

    for (0..try decoder.get_number_of_frames_remaining_in_buffer()) |_| {
        if (try decoder.flush()) |frame| {
            handle_frame(&frame);
        }
    }
}

/// Print YUV values of the frame.
fn handle_frame(frame: *const openh264.Frame) void {
    std.debug.print("yuv = ({}, {}, {})\n", .{
        frame.data.y[0],
        frame.data.u[0],
        frame.data.v[0],
    });
}
