const std = @import("std");

const openh264_bindings = @import("openh264_bindings");

/// This value is hardcoded since OpenH264 only supports YUV420 as input color
/// format.
const format = openh264_bindings.EVideoFormatType.i420;

/// YUV420 frame.
pub const Frame = struct {
    data: struct {
        y: []u8,
        u: []u8,
        v: []u8,
    },
    strides: struct {
        y: u32,
        u: u32,
        v: u32,
    },
    dims: struct {
        width: u32,
        height: u32,
    },
    /// Frame timestmap in milliseconds since start of frame.
    timestamp: u64 = 0,

    /// This can be used to handle allocation of the frame data in favor of
    /// doing it manually and having to deal with strides etc.
    pub fn alloc(width: u32, height: u32, allocator: std.mem.Allocator) !@This() {
        return .{
            .data = .{
                .y = try allocator.alloc(u8, width * height),
                .u = try allocator.alloc(u8, ((width / 2) * (height / 2))),
                .v = try allocator.alloc(u8, ((width / 2) * (height / 2))),
            },
            .strides = .{
                .y = width,
                .u = width / 2,
                .v = width / 2,
            },
            .dims = .{
                .width = width,
                .height = height,
            },
        };
    }

    pub fn free(self: *Frame, allocator: std.mem.Allocator) void {
        allocator.free(self.data.y);
        allocator.free(self.data.u);
        allocator.free(self.data.v);
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
    resolution: struct {
        width: u32,
        height: u32,
    },

    /// Store bitstream info. This variables holds the output bitstream of the
    /// encoder. It is reused for each frame to avoid unnecessary allocations.
    bitstream_info: openh264_bindings.SFrameBSInfo = std.mem.zeroes(openh264_bindings.SFrameBSInfo),

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

        var encoder = Encoder{
            .inner = inner.?,
            .resolution = .{
                .width = options.resolution.width,
                .height = options.resolution.height,
            },
            .allocator = allocator,
        };

        if (options.idr_interval) |value|
            try encoder.set_idr_interval(value);
        if (options.frame_rate) |value|
            try encoder.set_frame_rate(value);
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

    pub fn deinit(self: *const Encoder) void {
        _ = self.get_inner_vtable().Uninitialize.?(self.inner);
        openh264_bindings.WelsDestroySVCEncoder(self.inner);
    }

    /// Encode frame to H264 bitstream. The bitstream is written to the writer.
    pub fn encode(self: *Encoder, frame: *const Frame, writer: anytype) !void {
        std.debug.assert(frame.dims.width == self.resolution.width and frame.dims.height == self.resolution.height);

        const source_picture = openh264_bindings.SSourcePicture{
            .iColorFormat = format,
            .iStride = .{
                @intCast(frame.strides.y),
                @intCast(frame.strides.u),
                @intCast(frame.strides.v),
                0,
            },
            // these are fixed too
            .iPicWidth = @intCast(self.resolution.width),
            .iPicHeight = @intCast(self.resolution.height),
            .pData = .{
                frame.data.y.ptr,
                frame.data.u.ptr,
                frame.data.v.ptr,
                null,
            },
            .uiTimeStamp = @intCast(frame.timestamp),
        };

        try rc(self.get_inner_vtable().EncodeFrame.?(
            self.inner,
            &source_picture,
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

    pub fn get_frame_rate(self: *const Encoder) !f32 {
        const value: f32 = undefined;
        try rc(self.get_inner_vtable().GetOption.?(self.inner, .frame_rate, &value));
        return value;
    }

    pub fn set_frame_rate(self: *Encoder, value: f32) !void {
        try rc(self.get_inner_vtable().SetOption.?(self.inner, .frame_rate, &value));
    }

    // FIXME: target bitrate (OPTION_BITRATE) and max_bitrate
    // (OPTION_MAX_BITRATE) are left out because they require some extra logic
    // with the spatial and temporal layer stuff.

    pub fn get_rate_control_mode(self: *Encoder) !RateControlMode {
        const value_int: i32 = undefined;
        try rc(self.get_inner_vtable().GetOption.?(self.inner, .rc_mode, &value_int));
        return @enumFromInt(value_int);
    }

    pub fn set_rate_control_mode(self: *Encoder, value: RateControlMode) !void {
        const value_int: i32 = @intCast(@intFromEnum(value));
        try rc(self.get_inner_vtable().SetOption.?(self.inner, .rc_mode, &value_int));
    }

    pub fn get_rate_control_frame_skip(self: *Encoder) !bool {
        const value_int: i32 = undefined;
        try rc(self.get_inner_vtable().GetOption.?(self.inner, .rc_frame_skip, &value_int));
        return @as(bool, value_int);
    }

    pub fn set_rate_control_frame_skip(self: *Encoder, value: bool) !void {
        try rc(self.get_inner_vtable().SetOption.?(self.inner, .rc_frame_skip, &value));
    }

    pub fn get_padding(self: *Encoder) !bool {
        const value_int: i32 = undefined;
        try rc(self.get_inner_vtable().GetOption.?(self.inner, .padding, &value_int));
        return @as(bool, value_int);
    }

    pub fn set_padding(self: *Encoder, value: bool) !void {
        try rc(self.get_inner_vtable().SetOption.?(self.inner, .padding, &value));
    }

    pub fn set_enable_ssei(self: *Encoder, value: bool) !void {
        try rc(self.get_inner_vtable().SetOption.?(self.inner, .enable_ssei, &value));
    }

    pub fn get_enable_prefix_nal_adding(self: *Encoder) !bool {
        const value_int: i32 = undefined;
        try rc(self.get_inner_vtable().GetOption.?(self.inner, .enable_ssi, &value_int));
        return @as(bool, value_int);
    }

    pub fn set_enable_prefix_nal_adding(self: *Encoder, value: bool) !void {
        try rc(self.get_inner_vtable().SetOption.?(self.inner, .enable_prefix_nal_adding, &value));
    }

    pub fn get_parameter_set_strategy(self: *Encoder) !ParameterSetStrategy {
        const value_int: i32 = undefined;
        try rc(self.get_inner_vtable().GetOption.?(self.inner, .sps_pps_id_strategy, &value_int));
        return @enumFromInt(value_int);
    }

    pub fn set_parameter_set_strategy(self: *Encoder, value: ParameterSetStrategy) !void {
        try rc(self.get_inner_vtable().SetOption.?(self.inner, .sps_pps_id_strategy, &value));
    }

    inline fn get_inner_vtable(self: *const Encoder) *const openh264_bindings.ISVCEncoderVtbl {
        return self.inner.*.?;
    }
};

pub const VideoBitstreamType = openh264_bindings.VideoBitstreamType;

pub const ErrorConcealment = openh264_bindings.ErrorConIdc;

pub const Profile = openh264_bindings.EProfileIdc;

pub const Level = openh264_bindings.ELevelIdc;

pub const DecoderOptions = struct {
    trace_level: TraceLevel = .err,
    video_bitstream_type: VideoBitstreamType = .avc,
    error_concealment: ErrorConcealment = .disable,
};

/// OpenH264 video decoder.
/// This API is not thread-safe.
/// Call `decode` in a loop until data runs out. Then call
/// `set_end_of_stream(true)` to signal end of data. Then call `flush` for all
/// remaining frames in the buffer. You can use
/// get_number_of_frames_remaining_in_buffer to see the number of frames
/// remaining in the buffer.
pub const Decoder = struct {
    inner: *openh264_bindings.ISVCDecoder,

    allocator: std.mem.Allocator,

    pub fn init(allocator: std.mem.Allocator, options: DecoderOptions) !Decoder {
        var inner: ?*openh264_bindings.ISVCDecoder = null;
        try rc(openh264_bindings.WelsCreateDecoder(&inner));
        std.debug.assert(inner != null);
        const inner_vtable = inner.?.*.?;
        errdefer openh264_bindings.WelsDestroyDecoder(inner);

        try rc(inner_vtable.SetOption.?(inner, .trace_level, &options.trace_level));

        const parameters = openh264_bindings.SDecodingParam{
            .pFileNameRestructed = null,
            .uiCpuLoad = 0,
            .uiTargetDqLayer = 0,
            .eEcActiveIdc = options.error_concealment,
            .bParseOnly = false,
            .sVideoProperty = .{
                .size = 0,
                .eVideoBsType = options.video_bitstream_type,
            },
        };

        try rc(inner_vtable.Initialize.?(inner, &parameters));
        errdefer _ = inner_vtable.Uninitialize.?(inner);

        const decoder = Decoder{
            .inner = inner.?,
            .allocator = allocator,
        };

        return decoder;
    }

    pub fn deinit(self: *const Decoder) void {
        _ = self.get_inner_vtable().Uninitialize.?(self.inner);
        openh264_bindings.WelsDestroyDecoder(self.inner);
    }

    /// Slice should include start-code prefix.
    /// Frame is borrowed and only valid until the next call to decode.
    pub fn decode(self: *const Decoder, slice: []u8) !?Frame {
        var buffer_info = std.mem.zeroes(openh264_bindings.SBufferInfo);
        var frame_pointers: [3][*c]u8 = .{ null, null, null };

        const state = self.get_inner_vtable().DecodeFrameNoDelay.?(
            self.inner,
            slice.ptr,
            @intCast(slice.len),
            &frame_pointers,
            &buffer_info,
        );
        try rc_decoding_state(state);

        return frame(frame_pointers, &buffer_info);
    }

    /// Use to flush out last frames after end of stream.
    /// First signal end of stream with `set_end_of_stream`.
    pub fn flush(self: *const Decoder) !?Frame {
        var buffer_info = std.mem.zeroes(openh264_bindings.SBufferInfo);
        var frame_pointers: [3][*c]u8 = .{ null, null, null };

        const state = self.get_inner_vtable().Flush.?(self.inner, &frame_pointers, &buffer_info);
        try rc_decoding_state(state);

        return frame(frame_pointers, &buffer_info);
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

    pub fn get_profile(self: *const Decoder) !Profile {
        const value_int: i32 = undefined;
        try rc(self.get_inner_vtable().GetOption.?(self.inner, .profile, &value_int));
        return @enumFromInt(value_int);
    }

    pub fn get_level(self: *const Decoder) !Level {
        const value_int: i32 = undefined;
        try rc(self.get_inner_vtable().GetOption.?(self.inner, .level, &value_int));
        return @enumFromInt(value_int);
    }

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

    /// Construct a frame from the frame pointers and buffer info.
    /// Returns null if there is no frame.
    fn frame(
        frame_pointers: [3][*c]u8,
        buffer_info: *const openh264_bindings.SBufferInfo,
    ) ?Frame {
        switch (buffer_info.iBufferStatus) {
            1 => {},
            0 => return null, // no frame
            else => unreachable,
        }

        // if one of the frame pointers is null it means there is no decoded
        // frame (for reference see:
        // https://github.com/ralfbiedert/openh264-rs/blob/37b831126c66043b6ad1b9cc3b3d42756e2aa6a6/openh264/src/decoder.rs#L474)
        if (frame_pointers[0] == null or
            frame_pointers[1] == null or
            frame_pointers[2] == null) return null; // no frame

        std.debug.assert(buffer_info.UsrData.sSystemBuffer.iFormat == @intFromEnum(format));

        const width: u32 = @intCast(buffer_info.UsrData.sSystemBuffer.iWidth);
        const height: u32 = @intCast(buffer_info.UsrData.sSystemBuffer.iHeight);
        const stride_y: u32 = @intCast(buffer_info.UsrData.sSystemBuffer.iStride[0]);
        const stride_uv: u32 = @as(u32, @intCast(buffer_info.UsrData.sSystemBuffer.iStride[1])) / 2;
        return Frame{
            .data = .{
                .y = frame_pointers[0][0 .. height * stride_y],
                .u = frame_pointers[1][0 .. height * stride_uv],
                .v = frame_pointers[2][0 .. height * stride_uv],
            },
            .strides = .{
                .y = stride_y,
                .u = stride_uv,
                .v = stride_uv,
            },
            .dims = .{
                .width = width,
                .height = height,
            },
            .timestamp = buffer_info.uiInBsTimeStamp,
        };
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

    // decode-only errors:
    FramePending,
    RefLost,
    Bitstream,
    DepLayerLost,
    NoParameterSets,
    DataErrorConcealed,
    RefListNullPointers,
    InvalidArgument,
    DestinationBufferNeedExpand,
};

fn rc(return_code: anytype) Error!void {
    switch (@as(openh264_bindings.ReturnCode, @enumFromInt(return_code))) {
        .success => return,
        .init_para_error => return Error.InvalidParameter,
        .unknown_reason => return Error.UnknownReason,
        .malloc_meme_error => return Error.OutOfMemory,
        .init_expected => return Error.NotInitialized,
        .unsuporrted_data => return Error.UnsupportedData,
    }
}

fn rc_decoding_state(decoding_state: openh264_bindings.DecodingState) Error!void {
    switch (decoding_state) {
        .error_free => return,
        .frame_pending => return Error.FramePending,
        .ref_lost => return Error.RefLost,
        .bitstream_error => return Error.Bitstream,
        .dep_layer_lost => return Error.DepLayerLost,
        .no_param_sets => return Error.NoParameterSets,
        .data_error_concealed => return Error.DataErrorConcealed,
        .ref_list_null_ptrs => return Error.RefListNullPointers,
        .invalid_argument => return Error.InvalidArgument,
        .initial_opt_expected => return Error.NotInitialized,
        .out_of_memory => return Error.OutOfMemory,
        .dst_buf_need_expand => return Error.DestinationBufferNeedExpand,
    }
}
