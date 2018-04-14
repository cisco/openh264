project('openh264', ['c', 'cpp'],
  version : '1.7.0',
  meson_version : '>= 0.43',
  default_options : [ 'warning_level=1',
                      'buildtype=debugoptimized' ])

major_version = '4'

cpp = meson.get_compiler('cpp')

inc = include_directories([
  join_paths('codec', 'api', 'svc'),
  join_paths('codec', 'common', 'inc'),
])

processing_inc = include_directories([
  join_paths('codec', 'processing', 'interface'),
  join_paths('codec', 'processing', 'src', 'common'),
  join_paths('codec', 'processing', 'src', 'adaptivequantization'),
  join_paths('codec', 'processing', 'src', 'downsample'),
  join_paths('codec', 'processing', 'src', 'scrolldetection'),
  join_paths('codec', 'processing', 'src', 'vaacalc'),
])

console_common_inc = include_directories([
  join_paths('codec', 'console', 'common', 'inc')
])

decoder_inc = include_directories([
  join_paths('codec', 'decoder', 'core', 'inc'),
  join_paths('codec', 'decoder', 'plus', 'inc'),
])

encoder_inc = include_directories([
  join_paths('codec', 'encoder', 'core', 'inc'),
  join_paths('codec', 'encoder', 'plus', 'inc'),
])

asm_inc = join_paths(meson.current_source_dir(), 'codec', 'common', 'x86', '')

nasm = find_program('nasm', 'nasm.exe')

system = host_machine.system()
cpu_family = host_machine.cpu_family()

supported_arguments = cpp.get_supported_arguments([
  '-Wno-non-virtual-dtor',
  '-Wno-strict-aliasing'])

add_project_arguments(supported_arguments, language: 'cpp')

deps = [dependency('threads')]
c_args = []
cpp_args = []
asm_args = []
shared_lib_suffix = ''

if system == 'linux'
  if cpu_family == 'x86'
    asm_format = 'elf'
    asm_args += ['-DX86_32']
    add_project_arguments('-DX86_32_ASM', language: 'c')
  elif cpu_family == 'x86_64'
    asm_format = 'elf64'
    asm_args += ['-DUNIX64']
  else
    error ('FIXME: unhandled CPU family @0@ for Linux'.format(cpu_family))
  endif

  deps += [cpp.find_library('libm')]

  shared_lib_suffix = 'so.@0@'.format(meson.project_version())

  asm_args += ['-DHAVE_AVX2']
  add_project_arguments('-DHAVE_AVX2', language: 'cpp')
  add_project_arguments('-DHAVE_AVX2', '-DX86_ASM', language: 'c')
elif system == 'windows'
  if cpu_family == 'x86'
    asm_format = 'win32'
    asm_args += ['-DPREFIX', '-DX86_32']
  elif cpu_family == 'x86_64'
    asm_format = 'win64'
    asm_args += ['-DWIN64']
  else
    error ('FIXME: unhandled CPU family @0@ for Windows'.format(cpu_family))
  endif
else
  error ('FIXME: Unhandled system @0@'.format(system))
endif

asm_gen = generator(nasm,
  output : '@BASENAME@.o',
  arguments : [
    '-f', asm_format,
    '-i', asm_inc,
    '@INPUT@',
    '-o', '@OUTPUT@'] + asm_args)

api_headers = []
api_header_deps = []

subdir ('codec')
subdir ('test')

all_objects = [
  libcommon.extract_all_objects(),
  libprocessing.extract_all_objects(),
  libencoder.extract_all_objects(),
  libdecoder.extract_all_objects()
]

libopenh264_shared = shared_library('openh264',
  objects: all_objects,
  install: true,
  name_suffix: shared_lib_suffix,
  dependencies: deps)

libopenh264_static = static_library('openh264',
  objects: all_objects,
  install: true,
  dependencies: deps)

pkg_install_dir = '@0@/pkgconfig'.format(get_option('libdir'))

foreach t : ['', '-static']
  pkgconf = configuration_data()
  pkgconf.set('prefix', join_paths(get_option('prefix')))
  pkgconf.set('VERSION', meson.project_version())
  if t == '-static'
    do_install = false
    pkgconf.set('LIBS', '-lstdc++ -lpthread -lm')
    pkgconf.set('LIBS_PRIVATE', '')
  else
    do_install = true
    pkgconf.set('LIBS', '')
    pkgconf.set('LIBS_PRIVATE', '-lstdc++ -lpthread -lm')
  endif

  message('do_install: @0@'.format(do_install))

  configure_file(
    input: 'openh264.pc.in',
    output: 'openh264@0@.pc'.format(t),
    install: t == '-static' ? false : true,
    install_dir: t == '-static' ? '' : pkg_install_dir,
    configuration: pkgconf)
endforeach

openh264_dep = declare_dependency(
  link_with: libopenh264_shared,
  include_directories: include_directories('include'),
  dependencies: deps + api_header_deps)

if system == 'linux'
  filesymlinker = find_program('filesymlinker.py')
  res = custom_target('libopenh264.so',
    build_by_default: true,
    input : libopenh264_shared,
    output : 'libopenh264.so',
    command : [filesymlinker, '@INPUT@', '@OUTPUT@'])

  res = custom_target('libopenh264.so.@0@'.format(major_version),
    build_by_default: true,
    input : libopenh264_shared,
    output : 'libopenh264.so.@0@'.format(major_version),
    command : [filesymlinker, '@INPUT@', '@OUTPUT@'])

  meson.add_install_script('post_install.py', get_option('libdir'), shared_lib_suffix, major_version)
endif

filecopier = find_program('filecopier.py')
subdir ('include')
