test_sources = [
  'CWelsListTest.cpp',
  'ExpandPicture.cpp',
  'WelsThreadPoolTest.cpp',
  'WelsTaskListTest.cpp'
]

e = executable('test_common', test_sources,
        dependencies : gtest_main_dep,
        include_directories: [inc, test_inc],
        link_with: [libcommon])

test('common', e)
