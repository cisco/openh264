const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const build_config = makeBuildConfiguration(target.result);

    const lib = b.addStaticLibrary(.{
        .name = "openh264",
        .target = target,
        .optimize = optimize,
    });

    const common = addObjectLibraryCommon(b, &build_config, target, optimize);
    const processing = addObjectLibraryProcessing(b, &build_config, target, optimize);
    const encoder = addObjectLibraryEncoder(b, &build_config, target, optimize);
    const decoder = addObjectLibraryDecoder(b, &build_config, target, optimize);

    lib.addIncludePath(b.path("codec/api/wels"));
    lib.addObject(common);
    lib.addObject(processing);
    lib.addObject(encoder);
    lib.addObject(decoder);

    const bindings = b.addModule("openh264", .{
        .root_source_file = b.path("openh264.zig"),
        .target = target,
        .optimize = optimize,
    });
    bindings.linkLibrary(lib);

    lib.installHeader(b.path("codec/api/wels/codec_api.h"), "codec_api.h");
    lib.installHeader(b.path("codec/api/wels/codec_app_def.h"), "codec_app_def.h");
    lib.installHeader(b.path("codec/api/wels/codec_def.h"), "codec_def.h");
    lib.installHeader(b.path("codec/api/wels/codec_ver.h"), "codec_ver.h");

    b.installArtifact(lib);

    const example_rainbow = b.addExecutable(.{
        .name = "example_rainbow",
        .root_source_file = b.path("examples/rainbow.zig"),
        .target = target,
        .optimize = optimize,
    });
    example_rainbow.root_module.addImport("openh264", bindings);
    b.installArtifact(example_rainbow);
}

fn addObjectLibraryCommon(
    b: *std.Build,
    build_config: *const BuildConfiguration,
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
) *std.Build.Step.Compile {
    const obj = b.addObject(.{
        .name = "common",
        .target = target,
        .optimize = optimize,
    });

    obj.linkLibC();
    obj.linkLibCpp();

    obj.addIncludePath(b.path("codec/api/wels"));
    obj.addIncludePath(b.path("codec/common/inc"));

    obj.addCSourceFiles(.{
        .files = &.{
            "codec/common/src/common_tables.cpp",
            "codec/common/src/copy_mb.cpp",
            "codec/common/src/cpu.cpp",
            "codec/common/src/crt_util_safe_x.cpp",
            "codec/common/src/deblocking_common.cpp",
            "codec/common/src/expand_pic.cpp",
            "codec/common/src/intra_pred_common.cpp",
            "codec/common/src/mc.cpp",
            "codec/common/src/memory_align.cpp",
            "codec/common/src/sad_common.cpp",
            "codec/common/src/utils.cpp",
            "codec/common/src/welsCodecTrace.cpp",
            "codec/common/src/WelsTaskThread.cpp",
            "codec/common/src/WelsThread.cpp",
            "codec/common/src/WelsThreadLib.cpp",
            "codec/common/src/WelsThreadPool.cpp",
        },
        .flags = build_config.flags,
    });

    switch (target.result.cpu.arch) {
        .x86, .x86_64 => {
            obj.addIncludePath(b.path("codec/common/x86/inc"));

            addNasmFiles(b, obj, build_config, &.{
                "codec/common/x86/cpuid.asm",
                "codec/common/x86/dct.asm",
                "codec/common/x86/deblock.asm",
                "codec/common/x86/expand_picture.asm",
                "codec/common/x86/intra_pred_com.asm",
                "codec/common/x86/mb_copy.asm",
                "codec/common/x86/mc_chroma.asm",
                "codec/common/x86/mc_luma.asm",
                "codec/common/x86/satd_sad.asm",
                "codec/common/x86/vaa.asm",
            });
        },
        .arm => {
            obj.addIncludePath(b.path("codec/common/arm/inc"));

            obj.addCSourceFiles(.{
                .files = &.{
                    "codec/common/arm/copy_mb_neon.S",
                    "codec/common/arm/deblocking_neon.S",
                    "codec/common/arm/expand_picture_neon.S",
                    "codec/common/arm/intra_pred_common_neon.S",
                    "codec/common/arm/mc_neon.S",
                },
            });
        },
        .aarch64 => {
            obj.addIncludePath(b.path("codec/common/armcomm/inc"));

            obj.addCSourceFiles(.{
                .files = &.{
                    "codec/common/arm64/copy_mb_aarch64_neon.S",
                    "codec/common/arm64/deblocking_aarch64_neon.S",
                    "codec/common/arm64/expand_picture_aarch64_neon.S",
                    "codec/common/arm64/intra_pred_common_aarch64_neon.S",
                    "codec/common/arm64/mc_aarch64_neon.S",
                },
            });
        },
        .loongarch32, .loongarch64 => {
            obj.addIncludePath(b.path("codec/common/loongarch/inc"));

            obj.addCSourceFiles(.{
                .files = &.{
                    "codec/common/loongarch/copy_mb_lsx.c",
                    "codec/common/loongarch/deblock_lsx.c",
                    "codec/common/loongarch/intra_pred_com_lsx.c",
                    "codec/common/loongarch/intra_pred_com_lasx.c",
                    "codec/common/loongarch/mc_chroma_lsx.c",
                    "codec/common/loongarch/mc_horver_lsx.c",
                    "codec/common/loongarch/satd_sad_lasx.c",
                },
            });
        },
        else => {},
    }

    return obj;
}

fn addObjectLibraryProcessing(
    b: *std.Build,
    build_config: *const BuildConfiguration,
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
) *std.Build.Step.Compile {
    const obj = b.addObject(.{
        .name = "processing",
        .target = target,
        .optimize = optimize,
    });

    obj.linkLibC();
    obj.linkLibCpp();

    obj.addIncludePath(b.path("codec/api/wels"));
    obj.addIncludePath(b.path("codec/common/inc"));

    obj.addIncludePath(b.path("codec/processing/interface"));
    obj.addIncludePath(b.path("codec/processing/src/common"));
    obj.addIncludePath(b.path("codec/processing/src/adaptivequantization"));
    obj.addIncludePath(b.path("codec/processing/src/downsample"));
    obj.addIncludePath(b.path("codec/processing/src/scrolldetection"));
    obj.addIncludePath(b.path("codec/processing/src/vaacalc"));

    obj.addCSourceFiles(.{
        .files = &.{
            "codec/processing/src/adaptivequantization/AdaptiveQuantization.cpp",
            "codec/processing/src/backgrounddetection/BackgroundDetection.cpp",
            "codec/processing/src/common/memory.cpp",
            "codec/processing/src/common/WelsFrameWork.cpp",
            "codec/processing/src/common/WelsFrameWorkEx.cpp",
            "codec/processing/src/complexityanalysis/ComplexityAnalysis.cpp",
            "codec/processing/src/denoise/denoise.cpp",
            "codec/processing/src/denoise/denoise_filter.cpp",
            "codec/processing/src/downsample/downsample.cpp",
            "codec/processing/src/downsample/downsamplefuncs.cpp",
            "codec/processing/src/imagerotate/imagerotate.cpp",
            "codec/processing/src/imagerotate/imagerotatefuncs.cpp",
            "codec/processing/src/scenechangedetection/SceneChangeDetection.cpp",
            "codec/processing/src/scrolldetection/ScrollDetection.cpp",
            "codec/processing/src/scrolldetection/ScrollDetectionFuncs.cpp",
            "codec/processing/src/vaacalc/vaacalcfuncs.cpp",
            "codec/processing/src/vaacalc/vaacalculation.cpp",
        },
        .flags = build_config.flags,
    });

    switch (target.result.cpu.arch) {
        .x86, .x86_64 => {
            obj.addIncludePath(b.path("codec/common/x86/inc"));

            addNasmFiles(b, obj, build_config, &.{
                "codec/processing/src/x86/denoisefilter.asm",
                "codec/processing/src/x86/downsample_bilinear.asm",
                "codec/processing/src/x86/vaa.asm",
            });
        },
        .arm => {
            obj.addIncludePath(b.path("codec/common/arm/inc"));

            obj.addCSourceFiles(.{
                .files = &.{
                    "codec/processing/src/arm/adaptive_quantization.S",
                    "codec/processing/src/arm/down_sample_neon.S",
                    "codec/processing/src/arm/pixel_sad_neon.S",
                    "codec/processing/src/arm/vaa_calc_neon.S",
                },
            });
        },
        .aarch64 => {
            obj.addIncludePath(b.path("codec/common/armcomm/inc"));

            obj.addCSourceFiles(.{
                .files = &.{
                    "codec/processing/src/arm64/adaptive_quantization_aarch64_neon.S",
                    "codec/processing/src/arm64/down_sample_aarch64_neon.S",
                    "codec/processing/src/arm64/pixel_sad_aarch64_neon.S",
                    "codec/processing/src/arm64/vaa_calc_aarch64_neon.S",
                },
            });
        },
        .loongarch32, .loongarch64 => {
            obj.addIncludePath(b.path("codec/common/loongarch/inc"));

            obj.addCSourceFiles(.{
                .files = &.{
                    "codec/processing/src/loongarch/vaa_lsx.c",
                    "codec/processing/src/loongarch/vaa_lasx.c",
                },
            });
        },
        else => {},
    }

    return obj;
}

fn addObjectLibraryEncoder(
    b: *std.Build,
    build_config: *const BuildConfiguration,
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
) *std.Build.Step.Compile {
    const obj = b.addObject(.{
        .name = "encoder",
        .target = target,
        .optimize = optimize,
    });

    obj.linkLibC();
    obj.linkLibCpp();

    obj.addIncludePath(b.path("codec/api/wels"));
    obj.addIncludePath(b.path("codec/common/inc"));

    // NOTE: Encoder needs processing headers.
    obj.addIncludePath(b.path("codec/processing/interface"));
    obj.addIncludePath(b.path("codec/processing/src/common"));
    obj.addIncludePath(b.path("codec/processing/src/adaptivequantization"));
    obj.addIncludePath(b.path("codec/processing/src/downsample"));
    obj.addIncludePath(b.path("codec/processing/src/scrolldetection"));
    obj.addIncludePath(b.path("codec/processing/src/vaacalc"));

    obj.addIncludePath(b.path("codec/encoder/core/inc"));
    obj.addIncludePath(b.path("codec/encoder/plus/inc"));

    obj.addCSourceFiles(.{
        .files = &.{
            "codec/encoder/core/src/au_set.cpp",
            "codec/encoder/core/src/deblocking.cpp",
            "codec/encoder/core/src/decode_mb_aux.cpp",
            "codec/encoder/core/src/encode_mb_aux.cpp",
            "codec/encoder/core/src/encoder.cpp",
            "codec/encoder/core/src/encoder_data_tables.cpp",
            "codec/encoder/core/src/encoder_ext.cpp",
            "codec/encoder/core/src/get_intra_predictor.cpp",
            "codec/encoder/core/src/md.cpp",
            "codec/encoder/core/src/mv_pred.cpp",
            "codec/encoder/core/src/nal_encap.cpp",
            "codec/encoder/core/src/paraset_strategy.cpp",
            "codec/encoder/core/src/picture_handle.cpp",
            "codec/encoder/core/src/ratectl.cpp",
            "codec/encoder/core/src/ref_list_mgr_svc.cpp",
            "codec/encoder/core/src/sample.cpp",
            "codec/encoder/core/src/set_mb_syn_cabac.cpp",
            "codec/encoder/core/src/set_mb_syn_cavlc.cpp",
            "codec/encoder/core/src/slice_multi_threading.cpp",
            "codec/encoder/core/src/svc_base_layer_md.cpp",
            "codec/encoder/core/src/svc_enc_slice_segment.cpp",
            "codec/encoder/core/src/svc_encode_mb.cpp",
            "codec/encoder/core/src/svc_encode_slice.cpp",
            "codec/encoder/core/src/svc_mode_decision.cpp",
            "codec/encoder/core/src/svc_motion_estimate.cpp",
            "codec/encoder/core/src/svc_set_mb_syn_cabac.cpp",
            "codec/encoder/core/src/svc_set_mb_syn_cavlc.cpp",
            "codec/encoder/core/src/wels_preprocess.cpp",
            "codec/encoder/core/src/wels_task_base.cpp",
            "codec/encoder/core/src/wels_task_encoder.cpp",
            "codec/encoder/core/src/wels_task_management.cpp",
            "codec/encoder/plus/src/welsEncoderExt.cpp",
        },
        .flags = build_config.flags,
    });

    switch (target.result.cpu.arch) {
        .x86, .x86_64 => {
            obj.addIncludePath(b.path("codec/common/x86/inc"));

            addNasmFiles(b, obj, build_config, &.{
                "codec/encoder/core/x86/coeff.asm",
                "codec/encoder/core/x86/dct.asm",
                "codec/encoder/core/x86/intra_pred.asm",
                "codec/encoder/core/x86/matrix_transpose.asm",
                "codec/encoder/core/x86/memzero.asm",
                "codec/encoder/core/x86/quant.asm",
                "codec/encoder/core/x86/sample_sc.asm",
                "codec/encoder/core/x86/score.asm",
            });
        },
        .arm => {
            obj.addIncludePath(b.path("codec/common/arm/inc"));

            obj.addCSourceFiles(.{
                .files = &.{
                    "codec/encoder/core/arm/intra_pred_neon.S",
                    "codec/encoder/core/arm/intra_pred_sad_3_opt_neon.S",
                    "codec/encoder/core/arm/memory_neon.S",
                    "codec/encoder/core/arm/pixel_neon.S",
                    "codec/encoder/core/arm/reconstruct_neon.S",
                    "codec/encoder/core/arm/svc_motion_estimation.S",
                },
            });
        },
        .aarch64 => {
            obj.addIncludePath(b.path("codec/common/armcomm/inc"));

            obj.addCSourceFiles(.{
                .files = &.{
                    "codec/encoder/core/arm64/intra_pred_aarch64_neon.S",
                    "codec/encoder/core/arm64/intra_pred_sad_3_opt_aarch64_neon.S",
                    "codec/encoder/core/arm64/memory_aarch64_neon.S",
                    "codec/encoder/core/arm64/pixel_aarch64_neon.S",
                    "codec/encoder/core/arm64/reconstruct_aarch64_neon.S",
                    "codec/encoder/core/arm64/svc_motion_estimation_aarch64_neon.S",
                },
            });
        },
        .loongarch32, .loongarch64 => {
            obj.addIncludePath(b.path("codec/common/loongarch/inc"));

            obj.addCSourceFiles(.{
                .files = &.{
                    "codec/encoder/core/loongarch/quant_lsx.c",
                    "codec/encoder/core/loongarch/get_intra_predictor_lsx.c",
                    "codec/encoder/core/loongarch/dct_lasx.c",
                    "codec/encoder/core/loongarch/svc_motion_estimate_lsx.c",
                    "codec/encoder/core/loongarch/sample_lasx.c",
                },
            });
        },
        else => {},
    }

    return obj;
}

fn addObjectLibraryDecoder(
    b: *std.Build,
    build_config: *const BuildConfiguration,
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
) *std.Build.Step.Compile {
    const obj = b.addObject(.{
        .name = "decoder",
        .target = target,
        .optimize = optimize,
    });

    obj.linkLibC();
    obj.linkLibCpp();

    obj.addIncludePath(b.path("codec/api/wels"));
    obj.addIncludePath(b.path("codec/common/inc"));

    obj.addIncludePath(b.path("codec/decoder/core/inc"));
    obj.addIncludePath(b.path("codec/decoder/plus/inc"));

    obj.addCSourceFiles(.{
        .files = &.{
            "codec/decoder/core/src/au_parser.cpp",
            "codec/decoder/core/src/bit_stream.cpp",
            "codec/decoder/core/src/cabac_decoder.cpp",
            "codec/decoder/core/src/deblocking.cpp",
            "codec/decoder/core/src/decode_mb_aux.cpp",
            "codec/decoder/core/src/decode_slice.cpp",
            "codec/decoder/core/src/decoder.cpp",
            "codec/decoder/core/src/decoder_core.cpp",
            "codec/decoder/core/src/decoder_data_tables.cpp",
            "codec/decoder/core/src/error_concealment.cpp",
            "codec/decoder/core/src/fmo.cpp",
            "codec/decoder/core/src/get_intra_predictor.cpp",
            "codec/decoder/core/src/manage_dec_ref.cpp",
            "codec/decoder/core/src/memmgr_nal_unit.cpp",
            "codec/decoder/core/src/mv_pred.cpp",
            "codec/decoder/core/src/parse_mb_syn_cabac.cpp",
            "codec/decoder/core/src/parse_mb_syn_cavlc.cpp",
            "codec/decoder/core/src/pic_queue.cpp",
            "codec/decoder/core/src/rec_mb.cpp",
            "codec/decoder/plus/src/welsDecoderExt.cpp",
            "codec/decoder/core/src/wels_decoder_thread.cpp",
        },
        .flags = build_config.flags,
    });

    switch (target.result.cpu.arch) {
        .x86, .x86_64 => {
            obj.addIncludePath(b.path("codec/common/x86/inc"));

            addNasmFiles(b, obj, build_config, &.{
                "codec/decoder/core/x86/dct.asm",
                "codec/decoder/core/x86/intra_pred.asm",
            });
        },
        .arm => {
            obj.addIncludePath(b.path("codec/common/arm/inc"));

            obj.addCSourceFiles(.{
                .files = &.{
                    "codec/decoder/core/arm/block_add_neon.S",
                    "codec/decoder/core/arm/intra_pred_neon.S",
                },
            });
        },
        .aarch64 => {
            obj.addIncludePath(b.path("codec/common/armcomm/inc"));

            obj.addCSourceFiles(.{
                .files = &.{
                    "codec/decoder/core/arm64/block_add_aarch64_neon.S",
                    "codec/decoder/core/arm64/intra_pred_aarch64_neon.S",
                },
            });
        },
        .loongarch32, .loongarch64 => {
            obj.addIncludePath(b.path("codec/common/loongarch/inc"));

            obj.addCSourceFiles(.{
                .files = &.{
                    "codec/decoder/core/loongarch/mb_aux_lsx.c",
                },
            });
        },
        else => {},
    }

    return obj;
}

fn addNasmFiles(
    b: *std.Build,
    obj: *std.Build.Step.Compile,
    build_config: *const BuildConfiguration,
    comptime asm_source_files: []const []const u8,
) void {
    const nasm_dep = b.dependency("nasm", .{ .optimize = .ReleaseFast });
    const nasm_exe = nasm_dep.artifact("nasm");

    inline for (asm_source_files) |asm_source_file| {
        const asm_object_file = o_name: {
            const basename = std.fs.path.basename(asm_source_file);
            const ext = std.fs.path.extension(basename);
            break :o_name b.fmt("{s}{s}", .{ basename[0 .. basename.len - ext.len], ".o" });
        };

        const nasm_run = b.addRunArtifact(nasm_exe);
        nasm_run.addArgs(&.{ "-f", build_config.nasm_format });
        nasm_run.addArgs(&.{ "-i", "codec/common/x86" });
        nasm_run.addArg("-o");
        obj.addObjectFile(nasm_run.addOutputFileArg(asm_object_file));
        nasm_run.addArgs(build_config.nasm_flags);
        nasm_run.addFileArg(b.path(asm_source_file));
    }
}

const BuildConfiguration = struct {
    flags: []const []const u8,
    nasm_flags: []const []const u8,
    nasm_format: []const u8,
};

fn makeBuildConfiguration(target: std.Target) BuildConfiguration {
    return switch (target.os.tag) {
        .linux => switch (target.cpu.arch) {
            .x86 => .{
                .flags = &.{ "-DHAVE_AVX2", "-DX86_ASM", "-DX86_32_ASM" },
                .nasm_flags = &.{ "-DX86_32", "-DHAVE_AVX2" },
                .nasm_format = "elf",
            },
            .x86_64 => .{
                .flags = &.{ "-DHAVE_AVX2", "-DX86_ASM" },
                .nasm_flags = &.{ "-DUNIX64", "-DHAVE_AVX2" },
                .nasm_format = "elf64",
            },
            .arm => .{
                .flags = &.{"-DHAVE_NEON"},
                .nasm_flags = &.{},
                .nasm_format = "elf",
            },
            .aarch64 => .{
                .flags = &.{"-DHAVE_NEON_AARCH64"},
                .nasm_flags = &.{},
                .nasm_format = "elf64",
            },
            // NOTE: Adding support for longarch32, longarch64 and some other
            // architectures is relatively easy. See the original meson.build
            // file for more information:
            // https://github.com/cisco/openh264/blob/423eb2c3e47009f4e631b5e413123a003fdff1ed/meson.build#L89
            else => @panic("architecture not supported"),
        },
        .macos, .ios => switch (target.cpu.arch) {
            .x86 => .{
                .flags = &.{ "-DHAVE_AVX2", "-DX86_ASM", "-DX86_32_ASM" },
                .nasm_flags = &.{ "-DX86_32", "-DHAVE_AVX2" },
                .nasm_format = "macho32",
            },
            .x86_64 => .{
                .flags = &.{ "-DHAVE_AVX2", "-DX86_ASM" },
                .nasm_flags = &.{ "-DUNIX64", "-DHAVE_AVX2" },
                .nasm_format = "macho64",
            },
            .arm => .{
                .flags = &.{"-DHAVE_NEON"},
                .nasm_flags = &.{},
                .nasm_format = "macho32",
            },
            .aarch64 => .{
                .flags = &.{"-DHAVE_NEON_AARCH64"},
                .nasm_flags = &.{},
                .nasm_format = "macho64",
            },
            else => @panic("architecture not supported"),
        },
        .windows => switch (target.cpu.arch) {
            .x86 => .{
                .flags = &.{},
                .nasm_flags = &.{ "-DPREFIX", "-DX86_32" },
                .nasm_format = "win32",
            },
            .x86_64 => .{
                .flags = &.{},
                .nasm_flags = &.{"-DWIN64"},
                .nasm_format = "win64",
            },
            // NOTE: arm and aarch64 can be added but have some complications.
            // See meson.build:
            // https://github.com/cisco/openh264/blob/423eb2c3e47009f4e631b5e413123a003fdff1ed/meson.build#L124
            // Especially arm is complicated since it needs extra preprocessing
            // for the assembly files.
            else => @panic("architecture not supported on Windows (only x86 and x86_64 are supported)"),
        },
        else => @panic("os not supported"),
    };
}
