cpp_sources = [
  'src/common_tables.cpp',
  'src/copy_mb.cpp',
  'src/cpu.cpp',
  'src/crt_util_safe_x.cpp',
  'src/deblocking_common.cpp',
  'src/expand_pic.cpp',
  'src/intra_pred_common.cpp',
  'src/mc.cpp',
  'src/memory_align.cpp',
  'src/sad_common.cpp',
  'src/utils.cpp',
  'src/welsCodecTrace.cpp',
  'src/WelsTaskThread.cpp',
  'src/WelsThread.cpp',
  'src/WelsThreadLib.cpp',
  'src/WelsThreadPool.cpp',
]

asm_sources = [
  'x86/cpuid.asm',
  'x86/dct.asm',
  'x86/deblock.asm',
  'x86/expand_picture.asm',
  'x86/intra_pred_com.asm',
  'x86/mb_copy.asm',
  'x86/mc_chroma.asm',
  'x86/mc_luma.asm',
  'x86/satd_sad.asm',
  'x86/vaa.asm',
]

objs_asm = asm_gen.process(asm_sources)

libcommon = static_library('common', cpp_sources, objs_asm,
  include_directories: inc,
  dependencies: deps)
