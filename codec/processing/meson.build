cpp_sources = [
  'src/adaptivequantization/AdaptiveQuantization.cpp',
  'src/backgrounddetection/BackgroundDetection.cpp',
  'src/common/memory.cpp',
  'src/common/WelsFrameWork.cpp',
  'src/common/WelsFrameWorkEx.cpp',
  'src/complexityanalysis/ComplexityAnalysis.cpp',
  'src/denoise/denoise.cpp',
  'src/denoise/denoise_filter.cpp',
  'src/downsample/downsample.cpp',
  'src/downsample/downsamplefuncs.cpp',
  'src/imagerotate/imagerotate.cpp',
  'src/imagerotate/imagerotatefuncs.cpp',
  'src/scenechangedetection/SceneChangeDetection.cpp',
  'src/scrolldetection/ScrollDetection.cpp',
  'src/scrolldetection/ScrollDetectionFuncs.cpp',
  'src/vaacalc/vaacalcfuncs.cpp',
  'src/vaacalc/vaacalculation.cpp',
]

asm_sources = [
  'src/x86/denoisefilter.asm',
  'src/x86/downsample_bilinear.asm',
  'src/x86/vaa.asm',
]

objs_asm = asm_gen.process(asm_sources)

libprocessing = static_library('processing', cpp_sources, objs_asm,
  include_directories: [inc, processing_inc],
  dependencies: deps)
