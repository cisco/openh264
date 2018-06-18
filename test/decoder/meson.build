test_sources = [
  'DecUT_Deblock.cpp',
  'DecUT_DeblockCommon.cpp',
  'DecUT_DecExt.cpp',
  'DecUT_ErrorConcealment.cpp',
  'DecUT_IdctResAddPred.cpp',
  'DecUT_IntraPrediction.cpp',
  'DecUT_ParseSyntax.cpp',
  'DecUT_PredMv.cpp',
]

e = executable('test_decoder', test_sources,
        dependencies : gtest_main_dep,
        include_directories: [inc, test_inc, decoder_inc],
        link_with: [libcommon, libdecoder])

test('decoder', e,
  workdir: join_paths(meson.current_source_dir(), '..', '..'))
