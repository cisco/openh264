 {
     'target_defaults' : {
        'cflags': [],
        
        'configurations' : {
        'Debug' : {
          'cflags' : [ '-g' ],
          'xcode_settings' : {
           'GCC_OPTIMIZATION_LEVEL' : '0'
          }
        },
        'Release' : {
          'cflags' : [ '-O3' ],
         'xcode_settings' : {
           'GCC_OPTIMIZATION_LEVEL' : '3'
          }

        },
      }
     },
    'conditions': [
        [ '(OS == "linux") or (OS=="android")', {
                'variables': {
                    'common_defines' : [
                        '__GCC__',
                        'LINUX',
                        '__NO_CTYPE',
                        'HAVE_CACHE_LINE_ALIGN',
			'NO_DYNAMIC_VP',
                        'X86_ASM'
                        ],
                    'common_cflags' : [
                        '-fPIC',
                        '-m32'
                        ],
                    'common_ldflags' : [
                        '-m32',
                        ],
                    'common_libraries' : [
                        '-ldl',
                        '-lpthread'
                        ],
                    'enable_asm' : 'YES',
                    'asm_format' : 'elf',
                    'asm_prefix' : []
                    }
                }
          ],
          [ 'OS == "mac"', {
                'variables': {
                    'common_defines' : [
                        'HAVE_CACHE_LINE_ALIGN',
			'NO_DYNAMIC_VP',
#                        'X86_ASM'
                        ],
                    'common_libraries' : [
                        '-ldl',
                        '-lpthread'
                        ],
                    'common_cflags' : [
                        ],
                    'common_ldflags' : [
                         ''
                        ],
                    'enable_asm' : 'NO',
                    'asm_format' : 'macho',
                    'asm_prefix' : ['--prefix', '_']
                    },
                }
          ],
        ],
    'targets' : [
	{
            'target_name' : 'decoder',
	    'type' : 'static_library',
	    'include_dirs' : [
		'codec/api/svc',
		'codec/decoder/core/inc',
		'codec/decoder/plus/inc',
                ],
            'variables' : {
                'asm_path' : 'decoder_asm'
                },
	    'sources' : [
		'codec/decoder/core/src/au_parser.cpp',
		'codec/decoder/core/src/bit_stream.cpp',
		'codec/decoder/core/src/cpu.cpp',
		'codec/decoder/core/src/deblocking.cpp',
		'codec/decoder/core/src/decode_mb_aux.cpp',
		'codec/decoder/core/src/decoder.cpp',
		'codec/decoder/core/src/decoder_data_tables.cpp',
		'codec/decoder/core/src/expand_pic.cpp',
		'codec/decoder/core/src/fmo.cpp',
		'codec/decoder/core/src/get_intra_predictor.cpp',
		'codec/decoder/core/src/manage_dec_ref.cpp',
		'codec/decoder/core/src/mc.cpp',
		'codec/decoder/core/src/mem_align.cpp',
		'codec/decoder/core/src/memmgr_nal_unit.cpp',
		'codec/decoder/core/src/mv_pred.cpp',
		'codec/decoder/core/src/parse_mb_syn_cavlc.cpp',
		'codec/decoder/core/src/pic_queue.cpp',
		'codec/decoder/core/src/rec_mb.cpp',
		'codec/decoder/core/src/decode_slice.cpp',
		'codec/decoder/core/src/decoder_core.cpp',
		'codec/decoder/core/src/utils.cpp',
		'codec/decoder/plus/src/welsDecoderExt.cpp',
		'codec/decoder/plus/src/welsCodecTrace.cpp',
                ],
	    'defines' : [
                '<@(common_defines)',
                ],
            'cflags' : [
               '<@(common_cflags)',
               ],
            'xcode_settings': {
                 'ARCHS' : ['i386'],
                 'GCC_ENABLE_PASCAL_STRINGS': 'NO'
               },
            'conditions' : [
                [ '(enable_asm == "YES")', {
                        'sources' : [
                            'codec/decoder/core/asm/asm_inc.asm',
                            'codec/decoder/core/asm/block_add.asm',
                            'codec/decoder/core/asm/cpuid.asm',
                            'codec/decoder/core/asm/dct.asm',
                            'codec/decoder/core/asm/deblock.asm',
                            'codec/decoder/core/asm/expand_picture.asm',
                            'codec/decoder/core/asm/intra_pred.asm',
                            'codec/decoder/core/asm/mb_copy.asm',
                            'codec/decoder/core/asm/mc_chroma.asm',
                            'codec/decoder/core/asm/mc_luma.asm',
                            'codec/decoder/core/asm/memzero.asm'
                            ],
                        }
                  ]
                ],
            'includes' : [
                 'common/asm.gypi'
                 ],
            },

	{
            'target_name' : 'encoder',
	    'type' : 'static_library',
            'variables' : {
                'asm_path' : 'encoder_asm'
                },
	    'include_dirs' : [
		'codec/api/svc',
		'codec/encoder/core/inc',
		'codec/encoder/plus/inc',
                'codec/WelsThreadLib/api'
                ],
            'sources': [
                'codec/encoder/core/src/wels_preprocess.cpp',
                'codec/encoder/core/src/au_set.cpp',
                'codec/encoder/core/src/cpu.cpp',
                'codec/encoder/core/src/deblocking.cpp',
                'codec/encoder/core/src/decode_mb_aux.cpp',
                'codec/encoder/core/src/encode_mb_aux.cpp',
                'codec/encoder/core/src/encoder.cpp',
                'codec/encoder/core/src/encoder_data_tables.cpp',
                'codec/encoder/core/src/encoder_ext.cpp',
                'codec/encoder/core/src/expand_pic.cpp',
                'codec/encoder/core/src/get_intra_predictor.cpp',
                'codec/encoder/core/src/mc.cpp',
                'codec/encoder/core/src/md.cpp',
                'codec/encoder/core/src/memory_align.cpp',
                'codec/encoder/core/src/mv_pred.cpp',
                'codec/encoder/core/src/nal_encap.cpp',
                'codec/encoder/core/src/picture_handle.cpp',
                'codec/encoder/core/src/property.cpp',
                'codec/encoder/core/src/ratectl.cpp',
                'codec/encoder/core/src/ref_list_mgr_svc.cpp',
                'codec/encoder/core/src/sample.cpp',
                'codec/encoder/core/src/set_mb_syn_cavlc.cpp',
                'codec/encoder/core/src/slice_multi_threading.cpp',
                'codec/encoder/core/src/svc_enc_slice_segment.cpp',
                'codec/encoder/core/src/svc_base_layer_md.cpp',
                'codec/encoder/core/src/svc_encode_mb.cpp',
                'codec/encoder/core/src/svc_encode_slice.cpp',
                'codec/encoder/core/src/svc_mode_decision.cpp',
                'codec/encoder/core/src/svc_motion_estimate.cpp',
                'codec/encoder/core/src/svc_set_mb_syn_cavlc.cpp',
                'codec/encoder/core/src/utils.cpp',
#                'codec/WelsThreadLib/src/WelsThreadLib.cpp',
                'codec/encoder/plus/src/welsEncoderExt.cpp',
                'codec/encoder/plus/src/welsCodecTrace.cpp',
                ],
	    'defines' : [
                '<@(common_defines)',
                'WELS_SVC',
                'ENCODER_CORE',
                ],
            'cflags' : [
               '<@(common_cflags)',
               ],
            'xcode_settings': {
                 'ARCHS' : ['i386'],
                 'GCC_ENABLE_PASCAL_STRINGS': 'NO'
               },
            'conditions': [
                [ '(enable_asm == "YES")', {
                         'sources' : [
                             'codec/encoder/core/asm/asm_inc.asm',
                             'codec/encoder/core/asm/coeff.asm',
                             'codec/encoder/core/asm/cpuid.asm',
                             'codec/encoder/core/asm/dct.asm',
                             'codec/encoder/core/asm/deblock.asm',
                             'codec/encoder/core/asm/expand_picture.asm',
                             'codec/encoder/core/asm/intra_pred.asm',
                             'codec/encoder/core/asm/intra_pred_util.asm',
                             'codec/encoder/core/asm/mb_copy.asm',
                             'codec/encoder/core/asm/mc_chroma.asm',
                             'codec/encoder/core/asm/mc_luma.asm',
                             'codec/encoder/core/asm/memzero.asm',
                             'codec/encoder/core/asm/quant.asm',
                             'codec/encoder/core/asm/satd_sad.asm',
                             'codec/encoder/core/asm/score.asm',
                             'codec/encoder/core/asm/vaa.asm',
                             ]
                         }
                   ]
                 ],
            'includes' : [
                 'common/asm.gypi'
                 ],
            },
	    {	    
            'target_name' : 'welsvp',
	    'type' : 'static_library',
            'variables': {
                 'asm_path' : 'welsvp_asm',
                 },

	    'include_dirs' : [
		'codec/api/svc',
		'codec/decoder/core/inc',
		'codec/decoder/plus/inc',
                ],
	    'sources' : [
                'processing/src/adaptivequantization/AdaptiveQuantization.cpp',
                'processing/src/backgounddetection/BackgroundDetection.cpp',
                'processing/src/common/cpu.cpp',
                'processing/src/common/memory.cpp',
                'processing/src/common/thread.cpp',
                'processing/src/common/util.cpp',
                'processing/src/common/WelsFrameWork.cpp',
                'processing/src/common/WelsFrameWorkEx.cpp',
                'processing/src/complexityanalysis/ComplexityAnalysis.cpp',
                'processing/src/denoise/denoise.cpp',
                'processing/src/denoise/denoise_filter.cpp',
                'processing/src/downsample/downsample.cpp',
                'processing/src/downsample/downsamplefuncs.cpp',
                'processing/src/imagerotate/imagerotate.cpp',
                'processing/src/imagerotate/imagerotatefuncs.cpp',
                'processing/src/scenechangedetection/SceneChangeDetectionCommon.cpp',
                'processing/src/scenechangedetection/SceneChangeDetection.cpp',
                'processing/src/vaacalc/vaacalcfuncs.cpp',
                'processing/src/vaacalc/vaacalculation.cpp',
                ],
	    'defines' : [
                '<@(common_defines)',
                ],
            'cflags' : [
               '<@(common_cflags)',
               ],
               'xcode_settings': {
                 'ARCHS' : ['i386'],
                 'GCC_ENABLE_PASCAL_STRINGS': 'NO'
               },
            'conditions' : [
                [ '(enable_asm == "YES")', {
                  'sources' : [
                    'processing/src/asm/asm_inc.asm',
                    'processing/src/asm/cpuid.asm',
                    'processing/src/asm/denoisefilter.asm',
                    'processing/src/asm/downsample_bilinear.asm',
                    'processing/src/asm/intra_pred.asm',
                    'processing/src/asm/sad.asm',
                    'processing/src/asm/vaa.asm',
                  ],
                }
               ],
               ],
            'includes' : [
                 'common/asm.gypi'
                 ],
        },
        # Programs
        {
            'target_name' : 'decode264',
            'type' : 'executable',
            'include_dirs' : [
		'codec/api/svc',
                'codec/console/dec/inc/',
                'codec/decoder/core/inc'
                ],
            'sources' : [ 
                'codec/console/dec/src/d3d9_utils.cpp',
                'codec/console/dec/src/read_config.cpp',
                'codec/console/dec/src/h264dec.cpp'
                ],
            'cflags' : [
                '<@(common_cflags)'
                ],
             'xcode_settings': {
                 'ARCHS' : ['i386'],
                 'GCC_ENABLE_PASCAL_STRINGS': 'NO'
               },
            'ldflags' : [
                '<@(common_ldflags)'
                ],
            'link_settings' : {
                'libraries' : [
                    '<@(common_libraries)',
                    ]
                },
            'dependencies' : [
                'decoder'
                ]
            },
        {
            'target_name' : 'encode264',
            'type' : 'executable',
            'include_dirs' : [
		'codec/api/svc',
                'codec/console/enc/inc/',
                'codec/encoder/core/inc'
                ],
            'sources' : [ 
                'codec/console/enc/src/read_config.cpp',
                'codec/console/enc/src/welsenc.cpp'
                ],
            'cflags' : [
                '<@(common_cflags)'
                ],
            'xcode_settings': {
                 'ARCHS' : ['i386'],
                 'GCC_ENABLE_PASCAL_STRINGS': 'NO'
               },
            'ldflags' : [
                '<@(common_ldflags)'
                ],
            'link_settings' : {
                'libraries' : [
                    '<@(common_libraries)',
                    ]
                },
            'dependencies' : [
                'encoder',
		'welsvp'
                ]
            },
        ]
    }
