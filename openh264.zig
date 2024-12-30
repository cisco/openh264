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
    // TODO: store strides
    // TODO: store original resolution
    // TODO: store time

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
    enable_ssei: ?bool = null,
    enable_prefix_nal_adding: ?bool = null,
    parameter_set_strategy: ?ParameterSetStrategy = null,
};

/// OpenH264 video encoder.
/// This API is not thread-safe.
/// The encoder only does Constrained Baseline.
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

        // only yuv420 is supported so we must set this
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

    pub fn get_idr_interval(self: *const Encoder) !u32 {
        const value_int: i32 = undefined;
        try rc(self.get_inner_vtable().GetOption.?(self.inner, .idr_interval, &value_int));
        return @intCast(value_int);
    }

    pub fn set_idr_interval(self: *Encoder, value: u32) !void {
        const value_int: i32 = @intCast(value);
        try rc(self.get_inner_vtable().SetOption.?(self.inner, .idr_interval, &value_int));
    }

    // TODO: corresponding get options

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

pub const VideoBitstreamType = openh264_bindings.VideoBitstreamType;

pub const ErrorConcealment = openh264_bindings.ErrConIdc;

pub const DecoderOptions = struct {
    trace_level: TraceLevel = .err,
    video_bitstream_type: VideoBitstreamType = .avc,
    error_concealment: ?ErrorConcealment = null,
};

pub const Decoder = struct {
    inner: *openh264_bindings.ISVCDecoder,

    allocator: std.mem.Allocator,

    pub fn init(allocator: std.mem.Allocator, options: DecoderOptions) !Decoder {
        var inner: ?*openh264_bindings.ISVCDecoder = null;
        try rc(openh264_bindings.WelsCreateSVCDecoder(&inner));
        std.debug.assert(inner != null);
        const inner_vtable = inner.?.*.?;
        errdefer openh264_bindings.WelsDestroySVCDecoder(inner);

        try rc(inner_vtable.SetOption.?(inner, .trace_level, &options.trace_level));
        try rc(inner_vtable.SetOption.?(inner, .video_bitstream_type, &options.video_bitstream_type));

        try rc(inner_vtable.Initialize.?(inner));
        errdefer _ = inner_vtable.Uninitialize.?(inner);

        const decoder = Decoder{
            .inner = inner.?,
            .allocator = allocator,
        };

        if (options.error_concealment) |value|
            try decoder.set_error_concealment_idc(value);

        return decoder;
    }

    // TODO: actual decode
    // https://github.com/cisco/openh264/blob/423eb2c3e47009f4e631b5e413123a003fdff1ed/codec/api/wels/codec_api.h#L68

    /// Slice should include start-code prefix.
    /// Frame is borrowed and only valid until the next call to decode.
    pub fn decode(self: *const Decoder, slice: []u8) !Frame {

        // TODO: prepare buffers
        // look at this: https://github.com/ralfbiedert/openh264-rs/blob/37b831126c66043b6ad1b9cc3b3d42756e2aa6a6/openh264/src/decoder.rs#L339

        // DecodeFrameNoDelay: ?*const fn (?*ISVCDecoder, [*c]const u8, c_int, [*c][*c]u8, [*c]SBufferInfo) callconv(.C) DecodingState,

        var buffer_info = std.mem.zeroes(openh264_bindings.SBufferInfo);
        var frame_pointers: [3][*c]u8 = .{ null, null, null };
        const state = self.get_inner_vtable().DecodeFrameNoDelay(slice.ptr, @intCast(slice.len), &frame_pointers, &buffer_info);
        _ = state; // autofix
        // TODO: check state
        // TODO: what about iBufferStatus
        // TODO: handle case where frame_pointers a null (this is valid)

        // TODO: assert buffer_info.sSystemBuffer.iFormat

        const frame_height = buffer_info.sSystemBuffer.iHeight;
        const frame = Frame{
            .y = frame_pointers[0][0 .. frame_height * buffer_info.sSystemBuffer.iStride[0]],
            .u = frame_pointers[1][0 .. frame_height * buffer_info.sSystemBuffer.iStride[1] / 2],
            .v = frame_pointers[2][0 .. frame_height * buffer_info.sSystemBuffer.iStride[1] / 2],
        };

        // TODO: Flush?
        // TODO: signal end of stream?
        // TODO: pass on time
        //     uiInBsTimeStamp: c_ulonglong,
        //     uiOutYuvTimeStamp: c_ulonglong,

        return frame;
    }

    pub fn get_end_of_stream(self: *const Decoder) !bool {
        const value_int: i32 = undefined;
        try rc(self.get_inner_vtable().GetOption.?(self.inner, .end_of_stream, &value_int));
        return @as(bool, value_int);
    }

    pub fn set_end_of_stream(self: *Encoder, value: bool) !void {
        const value_int: i32 = @intCast(value);
        try rc(self.get_inner_vtable().SetOption.?(self.inner, .end_of_stream, &value_int));
    }

    pub fn get_vcl_nal(self: *const Decoder) !bool {
        const value_int: i32 = undefined;
        try rc(self.get_inner_vtable().GetOption.?(self.inner, .vcl_nal, &value_int));
        return @as(bool, value_int);
    }

    pub fn get_temporal_id(self: *const Decoder) !i32 {
        const value_int: i32 = undefined;
        try rc(self.get_inner_vtable().GetOption.?(self.inner, .temporal_id, &value_int));
        return value_int;
    }

    pub fn get_frame_num(self: *const Decoder) !usize {
        const value_int: i32 = undefined;
        try rc(self.get_inner_vtable().GetOption.?(self.inner, .frame_num, &value_int));
        return @intCast(value_int);
    }

    pub fn get_idr_pic_id(self: *const Decoder) !i32 {
        const value_int: i32 = undefined;
        try rc(self.get_inner_vtable().GetOption.?(self.inner, .idr_pic_id, &value_int));
        return value_int;
    }

    pub fn get_error_concealment_idc(self: *const Decoder) !ErrorConcealment {
        const value_int: i32 = undefined;
        try rc(self.get_inner_vtable().GetOption.?(self.inner, .error_con_idc, &value_int));
        return @enumFromInt(value_int);
    }

    pub fn set_error_concealment_idc(self: *Decoder, value: ErrorConcealment) !void {
        const value_int: i32 = @intCast(@intFromEnum(value));
        try rc(self.get_inner_vtable().SetOption.?(self.inner, .error_con_idc, &value_int));
    }

    // TODO
    // pub fn get_profile() !Profile {
    //     const value_int: i32 = undefined;
    //     try rc(self.get_inner_vtable().GetOption.?(self.inner, .profile, &value_int));
    //     return @enumFromInt(@intCast(value_int));
    // }

    // TODO
    // pub fn get_level() !Level {
    //     const value_int: i32 = undefined;
    //     try rc(self.get_inner_vtable().GetOption.?(self.inner, .level, &value_int));
    //     return @enumFromInt(@intCast(value_int));
    // }

    pub fn get_is_ref_pic(self: *const Decoder) !bool {
        const value_int: i32 = undefined;
        try rc(self.get_inner_vtable().GetOption.?(self.inner, .is_ref_pic, &value_int));
        return @as(bool, value_int);
    }

    pub fn get_number_of_frames_remaining_in_buffer(self: *const Decoder) !usize {
        const value_int: i32 = undefined;
        try rc(self.get_inner_vtable().GetOption.?(self.inner, .num_of_frames_remaining_in_buffer, &value_int));
        return @intCast(value_int);
    }

    pub fn get_number_of_threads(self: *const Decoder) !usize {
        const value_int: i32 = undefined;
        try rc(self.get_inner_vtable().GetOption.?(self.inner, .num_of_threads, &value_int));
        return @intCast(value_int);
    }

    pub fn deinit(self: *const Decoder) void {
        _ = self.get_inner_vtable().Uninitialize.?(self.inner);
        openh264_bindings.WelsDestroySVCDecoder(self.inner);
    }

    inline fn get_inner_vtable(self: *const Decoder) *const openh264_bindings.ISVCDecoderVtbl {
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
