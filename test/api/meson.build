test_sources = [
  'BaseDecoderTest.cpp',
  'BaseEncoderTest.cpp',
  'cpp_interface_test.cpp',
  'DataGenerator.cpp',
  'decode_api_test.cpp',
  'decode_encode_test.cpp',
  'decoder_ec_test.cpp',
  'decoder_test.cpp',
  'encode_decode_api_test.cpp',
  'encode_options_test.cpp',
  'encoder_test.cpp',
  'ltr_test.cpp',
  'simple_test.cpp',
  'c_interface_test.c',
  'sha1.c',
]

cpp_args = cpp.get_supported_arguments(['-Wno-dangling-else'])

e = executable('test_api', test_sources,
        dependencies : gtest_dep,
        include_directories: [inc, test_inc],
        cpp_args: cpp_args,
        link_with: [libcommon, libencoder, libdecoder, libprocessing])

test('api', e,
  timeout: 300,
  workdir: join_paths(meson.current_source_dir(), '..', '..'))
