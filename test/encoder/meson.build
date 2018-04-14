test_sources = [
  'EncUT_Cavlc.cpp',
  'EncUT_DecodeMbAux.cpp',
  'EncUT_EncoderExt.cpp',
  'EncUT_EncoderMb.cpp',
  'EncUT_EncoderMbAux.cpp',
  'EncUT_EncoderTaskManagement.cpp',
  'EncUT_ExpGolomb.cpp',
  'EncUT_GetIntraPredictor.cpp',
  'EncUT_InterfaceTest.cpp',
  'EncUT_MBCopy.cpp',
  'EncUT_MemoryAlloc.cpp',
  'EncUT_MemoryZero.cpp',
  'EncUT_MotionCompensation.cpp',
  'EncUT_MotionEstimate.cpp',
  'EncUT_ParameterSetStrategy.cpp',
  'EncUT_Reconstruct.cpp',
  'EncUT_Sample.cpp',
  'EncUT_SVC_me.cpp',
  'EncUT_SliceBufferReallocate.cpp',
]

cpp_args = cpp.get_supported_arguments(['-Wno-dangling-else'])

e = executable('test_encoder', test_sources + test_data_generator_sources + test_base_encoder_sources,
        dependencies : gtest_main_dep,
        include_directories: [inc, test_inc, encoder_inc, processing_inc],
	cpp_args: cpp_args,
        link_with: [libcommon, libencoder, libprocessing])

test('encoder', e,
  workdir: join_paths(meson.current_source_dir(), '..', '..'))
