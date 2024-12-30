pub const EncoderOption = enum(c_uint) {
    dataformat = 0,
    idr_interval = 1,
    svc_encode_param_base = 2,
    svc_encode_param_ext = 3,
    frame_rate = 4,
    bitrate = 5,
    max_bitrate = 6,
    inter_spatial_pred = 7,
    rc_mode = 8,
    rc_frame_skip = 9,
    padding = 10,
    profile = 11,
    level = 12,
    number_ref = 13,
    delivery_status = 14,
    ltr_recovery_request = 15,
    ltr_marking_feedback = 16,
    ltr_marking_period = 17,
    ltr = 18,
    complexity = 19,
    enable_ssei = 20,
    enable_prefix_nal_adding = 21,
    sps_pps_id_strategy = 22,
    current_path = 23,
    dump_file = 24,
    trace_level = 25,
    trace_callback = 26,
    trace_callback_context = 27,
    get_statistics = 28,
    statistics_log_interval = 29,
    is_lossless_link = 30,
    bits_vary_percentage = 31,
};

pub const ISVCEncoder = ?*const ISVCEncoderVtbl;

pub const ISVCEncoderVtbl = extern struct {
    /// @brief  Initialize the encoder
    /// @param  pParam  basic encoder parameter
    /// @return CM_RETURN: 0 - success; otherwise - failed;
    Initialize: ?*const fn (?*ISVCEncoder, ?*const SEncParamBase) callconv(.C) c_int,

    /// @brief  Initilaize encoder by using extension parameters.
    /// @param  pParam  extension parameter for encoder (NOTE: left out in bindings for now)
    /// @return CM_RETURN: 0 - success; otherwise - failed;
    InitializeExt: ?*const fn (?*ISVCEncoder, ?*const anyopaque) callconv(.C) c_int,

    /// @brief   Get the default extension parameters.
    ///          If you want to change some parameters of encoder, firstly you need to get the default encoding parameters,
    ///          after that you can change part of parameters you want to.
    /// @param   pParam  extension parameter for encoder (NOTE: left out in bindings for now)
    /// @return  CM_RETURN: 0 - success; otherwise - failed;
    GetDefaultParams: ?*const fn (?*ISVCEncoder, ?*anyopaque) callconv(.C) c_int,

    /// uninitialize the encoder
    Uninitialize: ?*const fn (?*ISVCEncoder) callconv(.C) c_int,

    /// @brief Encode one frame
    /// @param kpSrcPic the pointer to the source luminance plane
    ///        chrominance data:
    ///        CbData = kpSrc  +  m_iMaxPicWidth * m_iMaxPicHeight;
    ///        CrData = CbData + (m_iMaxPicWidth * m_iMaxPicHeight)/4;
    ///        the application calling this interface needs to ensure the data validation between the location
    /// @param pBsInfo output bit stream
    /// @return  0 - success; otherwise -failed;
    EncodeFrame: ?*const fn (?*ISVCEncoder, ?*const SSourcePicture, ?*SFrameBSInfo) callconv(.C) c_int,

    /// @brief  Encode the parameters from output bit stream
    /// @param  pBsInfo output bit stream
    /// @return 0 - success; otherwise - failed;
    EncodeParameterSets: ?*const fn (?*ISVCEncoder, ?*SFrameBSInfo) callconv(.C) c_int,

    /// @brief  Force encoder to encoder frame as IDR if bIDR set as true
    /// @param  bIDR true: force encoder to encode frame as IDR frame;false, return 1 and nothing to do
    /// @return 0 - success; otherwise - failed;
    ForceIntraFrame: ?*const fn (?*ISVCEncoder, bool) callconv(.C) c_int,

    /// @brief   Set option for encoder, detail option type, please refer to enumurate ENCODER_OPTION.
    /// @param   pOption option for encoder such as InDataFormat, IDRInterval, SVC Encode Param, Frame Rate, Bitrate,...
    /// @return  CM_RETURN: 0 - success; otherwise - failed;
    SetOption: ?*const fn (?*ISVCEncoder, EncoderOption, ?*const anyopaque) callconv(.C) c_int,

    /// @brief   Get option for encoder, detail option type, please refer to enumurate ENCODER_OPTION.
    /// @param   pOption option for encoder such as InDataFormat, IDRInterval, SVC Encode Param, Frame Rate, Bitrate,...
    /// @return  CM_RETURN: 0 - success; otherwise - failed;
    GetOption: ?*const fn (?*ISVCEncoder, EncoderOption, ?*anyopaque) callconv(.C) c_int,
};

/// @brief   Create encoder
/// @param   ppEncoder encoder
/// @return  0 - success; otherwise - failed;
pub extern fn WelsCreateSVCEncoder(ppEncoder: ?*?*ISVCEncoder) c_int;

/// @brief   Destroy encoder
/// @param   pEncoder encoder
/// @return  void
pub extern fn WelsDestroySVCEncoder(pEncoder: ?*ISVCEncoder) void;

pub const SDecoderCapability = extern struct {
    iProfileIdc: c_int,
    iProfileIop: c_int,
    iLevelIdc: c_int,
    iMaxMbps: c_int,
    iMaxFs: c_int,
    iMaxCpb: c_int,
    iMaxDpb: c_int,
    iMaxBr: c_int,
    bRedPicCap: bool,
};

/// @brief   Get the capability of decoder
/// @param   pDecCapability  decoder capability
/// @return  0 - success; otherwise - failed;
pub extern fn WelsGetDecoderCapability(pDecCapability: ?*SDecoderCapability) c_int;

pub const DecodingState = enum(c_uint) {
    error_free = 0,
    frame_pending = 1,
    ref_lost = 2,
    bitstream_error = 4,
    dep_layer_lost = 8,
    no_param_sets = 16,
    data_error_concealed = 32,
    ref_list_null_ptrs = 64,
    invalid_argument = 4096,
    initial_opt_expected = 8192,
    out_of_memory = 16384,
    dst_buf_need_expand = 32768,
};

pub const DecoderOption = enum(c_uint) {
    end_of_stream = 1,
    vcl_nal = 2,
    temporal_id = 3,
    frame_num = 4,
    idr_pic_id = 5,
    ltr_marking_flag = 6,
    ltr_marked_frame_num = 7,
    error_con_idc = 8,
    get_sar_info = 13,
    profile = 14,
    level = 15,
    is_ref_pic = 17,
    num_of_frames_remaining_in_buffer = 18,
    num_of_threads = 19,
};

pub const ISVCDecoder = ?*const ISVCDecoderVtbl;

pub const ISVCDecoderVtbl = extern struct {
    /// @brief  Initilaize decoder
    /// @param  pParam  parameter for decoder
    /// @return 0 - success; otherwise - failed;
    Initialize: ?*const fn (?*ISVCDecoder, ?*const SDecodingParam) callconv(.C) c_long,

    /// Uninitialize the decoder
    Uninitialize: ?*const fn (?*ISVCDecoder) callconv(.C) c_long,

    /// @brief   Decode one frame
    /// @param   pSrc the h264 stream to be decoded
    /// @param   iSrcLen the length of h264 stream
    /// @param   ppDst buffer pointer of decoded data (YUV)
    /// @param   pStride output stride
    /// @param   iWidth output width
    /// @param   iHeight output height
    /// @return  0 - success; otherwise -failed;
    DecodeFrame: ?*const fn (?*ISVCDecoder, [*c]const u8, c_int, [*c][*c]u8, [*c]c_int, [*c]c_int, [*c]c_int) callconv(.C) DecodingState,

    /// @brief    For slice level DecodeFrameNoDelay() (4 parameters input),
    ///           whatever the function return value is, the output data
    ///           of I420 format will only be available when pDstInfo->iBufferStatus == 1,.
    ///           This function will parse and reconstruct the input frame immediately if it is complete
    ///           It is recommended as the main decoding function for H.264/AVC format input
    /// @param   pSrc the h264 stream to be decoded
    /// @param   iSrcLen the length of h264 stream
    /// @param   ppDst buffer pointer of decoded data (YUV)
    /// @param   pDstInfo information provided to API(width, height, etc.)
    /// @return  0 - success; otherwise -failed;
    DecodeFrameNoDelay: ?*const fn (?*ISVCDecoder, [*c]const u8, c_int, [*c][*c]u8, [*c]SBufferInfo) callconv(.C) DecodingState,

    /// @brief    For slice level DecodeFrame2() (4 parameters input),
    ///           whatever the function return value is, the output data
    ///           of I420 format will only be available when pDstInfo->iBufferStatus == 1,.
    ///           (e.g., in multi-slice cases, only when the whole picture
    ///           is completely reconstructed, this variable would be set equal to 1.)
    /// @param   pSrc the h264 stream to be decoded
    /// @param   iSrcLen the length of h264 stream
    /// @param   ppDst buffer pointer of decoded data (YUV)
    /// @param   pDstInfo information provided to API(width, height, etc.)
    /// @return  0 - success; otherwise -failed;
    DecodeFrame2: ?*const fn (?*ISVCDecoder, [*c]const u8, c_int, [*c][*c]u8, [*c]SBufferInfo) callconv(.C) DecodingState,

    /// @brief   This function gets a decoded ready frame remaining in buffers after the last frame has been decoded.
    /// Use GetOption with option DECODER_OPTION_NUM_OF_FRAMES_REMAINING_IN_BUFFER to get the number of frames remaining in buffers.
    /// Note that it is only applicable for profile_idc != 66
    /// @param   ppDst buffer pointer of decoded data (YUV)
    /// @param   pDstInfo information provided to API(width, height, etc.)
    /// @return  0 - success; otherwise -failed;
    FlushFrame: ?*const fn (?*ISVCDecoder, [*c][*c]u8, [*c]SBufferInfo) callconv(.C) DecodingState,

    /// @brief   This function parse input bitstream only, and rewrite possible SVC syntax to AVC syntax
    /// @param   pSrc the h264 stream to be decoded
    /// @param   iSrcLen the length of h264 stream
    /// @param   pDstInfo bit stream info
    /// @return  0 - success; otherwise -failed;
    DecodeParser: ?*const fn (?*ISVCDecoder, [*c]const u8, c_int, ?*SParserBsInfo) callconv(.C) DecodingState,

    /// @brief   This API does not work for now!! This is for future use to support non-I420 color format output.
    /// @param   pSrc the h264 stream to be decoded
    /// @param   iSrcLen the length of h264 stream
    /// @param   pDst buffer pointer of decoded data (YUV)
    /// @param   iDstStride output stride
    /// @param   iDstLen bit stream info
    /// @param   iWidth output width
    /// @param   iHeight output height
    /// @param   iColorFormat output color format
    /// @return  to do ...
    DecodeFrameEx: ?*const fn (?*ISVCDecoder, [*c]const u8, c_int, [*c]u8, c_int, [*c]c_int, [*c]c_int, [*c]c_int, [*c]c_int) callconv(.C) DecodingState,

    /// @brief   Set option for decoder, detail option type, please refer to enumurate DECODER_OPTION.
    /// @param   pOption  option for decoder such as OutDataFormat, Eos Flag, EC method, ...
    /// @return  CM_RETURN: 0 - success; otherwise - failed;
    SetOption: ?*const fn (?*ISVCDecoder, DecoderOption, ?*anyopaque) callconv(.C) c_long,

    /// @brief   Get option for decoder, detail option type, please refer to enumurate DECODER_OPTION.
    /// @param   pOption  option for decoder such as OutDataFormat, Eos Flag, EC method, ...
    /// @return  CM_RETURN: 0 - success; otherwise - failed;
    GetOption: ?*const fn (?*ISVCDecoder, DecoderOption, ?*anyopaque) callconv(.C) c_long,
};

/// @brief   Create decoder
/// @param   ppDecoder decoder
/// @return  0 - success; otherwise - failed;
pub extern fn WelsCreateDecoder(ppDecoder: ?*?*ISVCDecoder) c_long;

/// @brief   Destroy decoder
/// @param   pDecoder  decoder
/// @return  void
pub extern fn WelsDestroyDecoder(pDecoder: ?*ISVCDecoder) void;

pub const OpenH264Version = extern struct {
    uMajor: c_uint,
    uMinor: c_uint,
    uRevision: c_uint,
    uReserved: c_uint,
};

/// @brief   Get codec version
///          Note, old versions of Mingw (GCC < 4.7) are buggy and use an
///          incorrect/different ABI for calling this function, making it
///          incompatible with MSVC builds.
/// @return  The linked codec version
pub extern fn WelsGetCodecVersion() OpenH264Version;

/// @brief   Get codec version
/// @param   pVersion  struct to fill in with the version
pub extern fn WelsGetCodecVersionEx(pVersion: ?*OpenH264Version) void;

pub const WelsTraceCallback = ?*const fn (?*anyopaque, c_int, [*c]const u8) callconv(.C) void;

pub const WelsLogLevel = enum(u32) {
    quiet = 0,
    err = 1,
    warning = 2,
    info = 4,
    debug = 8,
    detail = 16,
    reserved = 32,
};

pub const EParameterSetStrategy = enum(c_uint) {
    constant_id = 0,
    increasing_id = 1,
    sps_listing = 2,
    sps_listing_and_pps_increasing = 3,
    sps_pps_listing = 6,
};

/// Note that is seems only camera_video_real_time and screen_content_real_time
/// are supported for encoding.
pub const EUsageType = enum(c_uint) {
    camera_video_real_time = 0,
    screen_content_real_time = 1,
    camera_video_non_real_time = 2,
    screen_content_non_real_time = 3,
    input_content_type_all = 4,
};

pub const EVideoFormatType = enum(c_int) {
    rgb = 1,
    rgba = 2,
    rgb555 = 3,
    rgb565 = 4,
    bgr = 5,
    bgra = 6,
    abgr = 7,
    argb = 8,
    yuy2 = 20,
    yvyu = 21,
    uyvy = 22,
    i420 = 23,
    yv12 = 24,
    internal = 25,
    nv12 = 26,
};

pub const EVideoFrameType = enum(c_uint) {
    invalid = 0,
    idr = 1,
    i = 2,
    p = 3,
    skip = 4,
    ip_mixed = 5,
};

pub const ErrorConIdc = enum(c_uint) {
    disable = 0,
    frame_copy = 1,
    slice_copy = 2,
    frame_copy_cross_idr = 3,
    slice_copy_cross_idr = 4,
    slice_copy_cross_idr_freeze_res_change = 5,
    slice_mv_copy_cross_idr = 6,
    slice_mv_copy_cross_idr_freeze_res_change = 7,
};

pub const RCMode = enum(c_int) {
    quality_mode = 0,
    bitrate_mode = 1,
    bufferbased_mode = 2,
    timestamp_mode = 3,
    bitrate_mode_post_skip = 4,
    off_mode = -1,
};

pub const ReturnCode = enum(c_int) {
    success = 0,
    init_para_error = 1,
    unknown_reason = 2,
    malloc_meme_error = 3,
    init_expected = 4,
    unsuporrted_data = 5,
};

pub const VideoBitstreamType = enum(c_uint) {
    avc = 0,
    svc = 1,
};

pub const SBufferInfo = extern struct {
    iBufferStatus: c_int,
    uiInBsTimeStamp: c_ulonglong,
    uiOutYuvTimeStamp: c_ulonglong,
    UsrData: extern union {
        sSystemBuffer: extern struct {
            iWidth: c_int,
            iHeight: c_int,
            iFormat: c_int,
            iStride: [2]c_int,
        },
    },
    pDst: [3][*c]u8,
};

pub const SDecodingParam = extern struct {
    pFileNameRestructed: [*c]u8,
    uiCpuLoad: c_uint,
    uiTargetDqLayer: u8,
    eEcActiveIdc: ErrorConIdc,
    bParseOnly: bool,
    sVideoProperty: SVideoProperty,
};

pub const SEncParamBase = extern struct {
    iUsageType: EUsageType,
    iPicWidth: c_int,
    iPicHeight: c_int,
    iTargetBitrate: c_int,
    iRCMode: RCMode,
    fMaxFrameRate: f32,
};

pub const SFrameBSInfo = extern struct {
    iLayerNum: c_int,
    sLayerInfo: [128]SLayerBSInfo,
    eFrameType: EVideoFrameType,
    iFrameSizeInBytes: c_int,
    uiTimeStamp: c_longlong,
};

pub const SLayerBSInfo = extern struct {
    uiTemporalId: u8,
    uiSpatialId: u8,
    uiQualityId: u8,
    eFrameType: EVideoFrameType,
    uiLayerType: u8,
    iSubSeqId: c_int,
    iNalCount: c_int,
    pNalLengthInByte: [*c]c_int,
    pBsBuf: [*c]u8,
};

pub const SParserBsInfo = extern struct {
    iNalNum: c_int,
    pNalLenInByte: [*c]c_int,
    pDstBuff: [*c]u8,
    iSpsWidthInPixel: c_int,
    iSpsHeightInPixel: c_int,
    uiInBsTimeStamp: c_ulonglong,
    uiOutBsTimeStamp: c_ulonglong,
};

pub const SSourcePicture = extern struct {
    iColorFormat: EVideoFormatType,
    iStride: [4]c_int,
    pData: [4][*c]u8,
    iPicWidth: c_int,
    iPicHeight: c_int,
    uiTimeStamp: c_longlong,
};

pub const SVideoProperty = extern struct {
    size: c_uint,
    eVideoBsType: VideoBitstreamType = default_video_bistream_type,
};
