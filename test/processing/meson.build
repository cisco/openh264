test_sources = [
  'ProcessUT_AdaptiveQuantization.cpp',
  'ProcessUT_DownSample.cpp',
  'ProcessUT_ScrollDetection.cpp',
  'ProcessUT_VaaCalc.cpp',
]

e = executable('test_processing', test_sources + test_data_generator_sources,
        dependencies : gtest_main_dep,
        include_directories: [inc, test_inc, processing_inc],
        link_with: [libcommon, libprocessing])

test('processing', e)
