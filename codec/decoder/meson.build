cpp_sources = [
  'core/src/au_parser.cpp',
  'core/src/bit_stream.cpp',
  'core/src/cabac_decoder.cpp',
  'core/src/deblocking.cpp',
  'core/src/decode_mb_aux.cpp',
  'core/src/decode_slice.cpp',
  'core/src/decoder.cpp',
  'core/src/decoder_core.cpp',
  'core/src/decoder_data_tables.cpp',
  'core/src/error_concealment.cpp',
  'core/src/fmo.cpp',
  'core/src/get_intra_predictor.cpp',
  'core/src/manage_dec_ref.cpp',
  'core/src/memmgr_nal_unit.cpp',
  'core/src/mv_pred.cpp',
  'core/src/parse_mb_syn_cabac.cpp',
  'core/src/parse_mb_syn_cavlc.cpp',
  'core/src/pic_queue.cpp',
  'core/src/rec_mb.cpp',
  'plus/src/welsDecoderExt.cpp',
  'core/src/wels_decoder_thread.cpp',
]

asm_sources = [
  'core/x86/dct.asm',
  'core/x86/intra_pred.asm',
]

objs_asm = asm_gen.process(asm_sources)

libdecoder = static_library('decoder', cpp_sources, objs_asm,
  include_directories: [inc, decoder_inc],
  dependencies: deps)
