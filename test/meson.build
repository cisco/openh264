if not get_option('tests').disabled()
  gtest_main_dep = dependency('gtest', main : true, fallback: ['gtest', 'gtest_main_dep'], required: true)
  gtest_dep = dependency('gtest', fallback: ['gtest', 'gtest_dep'], required: true)

  test_inc = include_directories('.')

  test_data_generator_sources = files('api/DataGenerator.cpp')
  test_base_encoder_sources = files('api/BaseEncoderTest.cpp')

  if gtest_dep.found()
    subdir('api')
    subdir('common')
    subdir('decoder')
    subdir('encoder')
    subdir('processing')
  endif
endif