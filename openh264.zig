const std = @import("std");

const openh264_bindings = @import("openh264_bindings");

/// This value is hardcoded since OpenH264 only supports YUV420 as input color
/// format.
const format = openh264_bindings.EVideoFormatType.i420;

/// YUV420 frame.
pub const Frame = struct {
    y: []u8,
    u: []u8,
    v: []u8,

    pub fn init(width: usize, height: usize, allocator: std.mem.Allocator) !@This() {
        return .{
            .y = try allocator.alloc(u8, width * height),
            .u = try allocator.alloc(u8, ((width / 2) * (height / 2))),
            .v = try allocator.alloc(u8, ((width / 2) * (height / 2))),
        };
    }

    pub fn deinit(self: *Frame, allocator: std.mem.Allocator) void {
        allocator.free(self.y);
        allocator.free(self.u);
        allocator.free(self.v);
    }
};

pub const ParameterSetStrategy = openh264_bindings.EParameterSetStrategy;

/// Usage type for encoder.
///
/// Use `camera_video_real_time` for camera video real-time communication.
/// Use `screen_content_real_time` for screen content real-time communication.
///
/// Other values are not supported.
pub const UsageType = openh264_bindings.EUsageType;

/// Rate control mode for encoder.
pub const RateControlMode = openh264_bindings.RCMode;

/// Minimum trace level. This controls OpenH264 output. Set to `quiet` to
/// suppress output.
pub const TraceLevel = openh264_bindings.WelsLogLevel;

pub const EncoderOptions = struct {
    usage_type: UsageType = UsageType.camera_video_real_time,
    resolution: struct {
        width: u32,
        height: u32,
    },
    max_frame_rate: f32 = 30.0,
    target_bitrate: u32 = 5_000_000,
    rate_control_mode: RateControlMode = .quality_mode,
    trace_level: TraceLevel = .err,

    // options that can only be set through SetOption:
    idr_interval: ?u32 = null,
    frame_rate: ?f32 = null,
    bitrate: ?u32 = null,
    max_bitrate: ?u32 = null,
    rate_control_frame_skip: ?bool = null,
    padding: ?bool = null,
    // FIXME: Support profile and level later which is complicated since there is
    // something called spatial layers which I'll have to figure out.
    enable_ssei: ?bool = null,
    enable_prefix_nal_adding: ?bool = null,
    parameter_set_strategy: ?ParameterSetStrategy = null,
};

/// H254 video encoder.
/// This API is not thread-safe.
pub const Encoder = struct {
    inner: *openh264_bindings.ISVCEncoder,

    /// Store bitstream info. This variables holds the output bitstream of the
    /// encoder. It is reused for each frame to avoid unnecessary allocations.
    bitstream_info: openh264_bindings.SFrameBSInfo = std.mem.zeroes(openh264_bindings.SFrameBSInfo),
    /// Holds the YUV420 frame data. This is reused for each frame.
    source_picture: openh264_bindings.SSourcePicture,

    allocator: std.mem.Allocator,

    pub fn init(allocator: std.mem.Allocator, options: EncoderOptions) !Encoder {
        var inner: ?*openh264_bindings.ISVCEncoder = null;
        try rc(openh264_bindings.WelsCreateSVCEncoder(&inner));
        std.debug.assert(inner != null);
        const inner_vtable = inner.?.*.?;
        errdefer openh264_bindings.WelsDestroySVCEncoder(inner);

        try rc(inner_vtable.SetOption.?(inner, .trace_level, &options.trace_level));

        // other usage types are not supported by openh264
        std.debug.assert(options.usage_type == UsageType.camera_video_real_time or
            options.usage_type == UsageType.screen_content_real_time);

        const parameters = openh264_bindings.SEncParamBase{
            .iUsageType = options.usage_type,
            .iPicWidth = @intCast(options.resolution.width),
            .iPicHeight = @intCast(options.resolution.height),
            .fMaxFrameRate = options.max_frame_rate,
            .iTargetBitrate = @intCast(options.target_bitrate),
            .iRCMode = options.rate_control_mode,
        };
        try rc(inner_vtable.Initialize.?(inner, &parameters));
        errdefer _ = inner_vtable.Uninitialize.?(inner);

        // only yuv420 is supported
        try rc(inner_vtable.SetOption.?(inner, .dataformat, &format));

        // pre-fill SSourcePicture values
        const source_picture = openh264_bindings.SSourcePicture{
            .iColorFormat = format,
            // these are always the strides for YUV420 input
            .iStride = .{
                @intCast(options.resolution.width),
                @intCast(options.resolution.width / 2),
                @intCast(options.resolution.width / 2),
                0,
            },
            // these are fixed too
            .iPicWidth = parameters.iPicWidth,
            .iPicHeight = parameters.iPicHeight,
            // we will fill the actual frame data during encode
            .pData = .{ null, null, null, null },
            .uiTimeStamp = 0,
        };

        var encoder = Encoder{
            .inner = inner.?,
            .source_picture = source_picture,
            .allocator = allocator,
        };

        if (options.idr_interval) |value|
            try encoder.set_idr_interval(value);
        if (options.frame_rate) |value|
            try encoder.set_frame_rate(value);
        if (options.bitrate) |value|
            try encoder.set_bitrate(value);
        if (options.max_bitrate) |value|
            try encoder.set_max_bitrate(value);
        if (options.rate_control_frame_skip) |value|
            try encoder.set_rate_control_frame_skip(value);
        if (options.padding) |value|
            try encoder.set_padding(value);
        if (options.enable_ssei) |value|
            try encoder.set_enable_ssei(value);
        if (options.enable_prefix_nal_adding) |value|
            try encoder.set_enable_prefix_nal_adding(value);
        if (options.parameter_set_strategy) |value|
            try encoder.set_parameter_set_strategy(value);

        return encoder;
    }

    /// Encode frame to H264 bitstream. The bitstream is written to the writer.
    pub fn encode(self: *Encoder, frame: *const Frame, writer: anytype) !void {
        self.source_picture.pData = .{ frame.y.ptr, frame.u.ptr, frame.v.ptr, null };

        try rc(self.get_inner_vtable().EncodeFrame.?(
            self.inner,
            &self.source_picture,
            &self.bitstream_info,
        ));

        if (self.bitstream_info.eFrameType == .invalid) return error.InvalidFrame;
        if (self.bitstream_info.eFrameType == .skip) return;

        for (0..@intCast(self.bitstream_info.iLayerNum)) |layer_index| {
            const layer = self.bitstream_info.sLayerInfo[layer_index];
            var nal_offset: usize = 0;
            for (0..@intCast(layer.iNalCount)) |nal_index| {
                const nal_len: usize = @intCast(layer.pNalLengthInByte[nal_index]);
                const nal_data = layer.pBsBuf[nal_offset .. nal_offset + nal_len];
                try writer.writeAll(nal_data);
                nal_offset += nal_len;
            }
        }
    }

    pub fn set_idr_interval(self: *Encoder, value: u32) !void {
        const value_int: i32 = @intCast(value);
        try rc(self.get_inner_vtable().SetOption.?(self.inner, .idr_interval, &value_int));
    }

    pub fn set_frame_rate(self: *Encoder, value: f32) !void {
        try rc(self.get_inner_vtable().SetOption.?(self.inner, .frame_rate, &value));
    }

    pub fn set_bitrate(self: *Encoder, value: u32) !void {
        const value_int: i32 = @intCast(value);
        try rc(self.get_inner_vtable().SetOption.?(self.inner, .bitrate, &value_int));
    }

    pub fn set_max_bitrate(self: *Encoder, value: u32) !void {
        const value_int: i32 = @intCast(value);
        try rc(self.get_inner_vtable().SetOption.?(self.inner, .max_bitrate, &value_int));
    }

    pub fn set_rate_control_mode(self: *Encoder, value: RateControlMode) !void {
        const value_int: i32 = @intCast(@intFromEnum(value));
        try rc(self.get_inner_vtable().SetOption.?(self.inner, .rc_mode, &value_int));
    }

    pub fn set_rate_control_frame_skip(self: *Encoder, value: bool) !void {
        try rc(self.get_inner_vtable().SetOption.?(self.inner, .rc_mode, &value));
    }

    pub fn set_padding(self: *Encoder, value: bool) !void {
        try rc(self.get_inner_vtable().SetOption.?(self.inner, .padding, &value));
    }

    // FIXME: Support profile and level later which is complicated since there is
    // something called spatial layers which I'll have to figure out.
    // * set_profile
    // * set_level

    pub fn set_enable_ssei(self: *Encoder, value: bool) !void {
        try rc(self.get_inner_vtable().SetOption.?(self.inner, .enable_ssei, &value));
    }

    pub fn set_enable_prefix_nal_adding(self: *Encoder, value: bool) !void {
        try rc(self.get_inner_vtable().SetOption.?(self.inner, .enable_prefix_nal_adding, &value));
    }

    pub fn set_parameter_set_strategy(self: *Encoder, value: ParameterSetStrategy) !void {
        try rc(self.get_inner_vtable().SetOption.?(self.inner, .sps_pps_id_strategy, &value));
    }

    pub fn deinit(self: *const Encoder) void {
        _ = self.get_inner_vtable().Uninitialize.?(self.inner);
        openh264_bindings.WelsDestroySVCEncoder(self.inner);
    }

    inline fn get_inner_vtable(self: *const Encoder) *const openh264_bindings.ISVCEncoderVtbl {
        return self.inner.*.?;
    }
};

pub const Error = error{
    InvalidParameter,
    UnknownReason,
    OutOfMemory,
    NotInitialized,
    UnsupportedData,
    InvalidFrame,
};

fn rc(return_code: c_int) Error!void {
    switch (@as(openh264_bindings.ReturnCode, @enumFromInt(return_code))) {
        .success => return,
        .init_para_error => return Error.InvalidParameter,
        .unknown_reason => return Error.UnknownReason,
        .malloc_meme_error => return Error.OutOfMemory,
        .init_expected => return Error.NotInitialized,
        .unsuporrted_data => return Error.UnsupportedData,
    }
}
