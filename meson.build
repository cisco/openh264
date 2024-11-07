project('openh264', ['c', 'cpp'],
  version : '2.5.0',
  meson_version : '>= 0.52',
  default_options : [ 'warning_level=1',
                      'buildtype=debugoptimized' ])

major_version = '7'

cpp = meson.get_compiler('cpp')

inc = include_directories([
  join_paths('codec', 'api', 'wels'),
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

system = host_machine.system()
cpu_family = host_machine.cpu_family()

supported_arguments = cpp.get_supported_arguments([
  '-Wno-non-virtual-dtor',
  '-Werror',
  '-Wunused-but-set-variable',
  '-Wno-strict-aliasing'])

add_project_arguments(supported_arguments, language: 'cpp')

deps = [dependency('threads')]
c_args = []
cpp_args = []
asm_args = []
asm_inc = []
casm_inc = []
cpp_lib = '-lstdc++'

libm_dep = cpp.find_library('m', required : false)
deps += [libm_dep]

if ['linux', 'android', 'ios', 'darwin'].contains(system)
  asm_format32 = 'elf'
  asm_format64 = 'elf64'
  if ['ios', 'darwin'].contains(system)
    asm_format32 = 'macho32'
    asm_format64 = 'macho64'
  endif
  if cpu_family == 'x86'
    asm_format = asm_format32
    asm_args += ['-DX86_32', '-DHAVE_AVX2']
    add_project_arguments('-DHAVE_AVX2', language: 'cpp')
    add_project_arguments('-DHAVE_AVX2', '-DX86_ASM', '-DX86_32_ASM', language: 'c')
    asm_inc = join_paths(meson.current_source_dir(), 'codec', 'common', 'x86', '')
  elif cpu_family == 'x86_64'
    asm_format = asm_format64
    asm_args += ['-DUNIX64', '-DHAVE_AVX2']
    add_project_arguments('-DHAVE_AVX2', language: 'cpp')
    add_project_arguments('-DHAVE_AVX2', '-DX86_ASM', language: 'c')
    asm_inc = join_paths(meson.current_source_dir(), 'codec', 'common', 'x86', '')
  elif cpu_family == 'arm'
    asm_format = asm_format32
    add_project_arguments('-DHAVE_NEON', language: 'c')
    add_project_arguments('-DHAVE_NEON', language: 'c')
    casm_inc = include_directories(join_paths('codec', 'common', 'arm'))
  elif cpu_family == 'aarch64'
    asm_format = asm_format64
    add_project_arguments('-DHAVE_NEON_AARCH64', language: 'c')
    add_project_arguments('-DHAVE_NEON_AARCH64', language: 'cpp')
    casm_inc = include_directories(join_paths('codec', 'common', 'arm64'))
  elif cpu_family == 'loongarch32'
    asm_format = asm_format32
    add_project_arguments('-mlsx', '-DHAVE_LSX', '-mlasx', '-DHAVE_LASX', language: 'c')
    add_project_arguments('-DHAVE_LSX', '-DHAVE_LASX', language: 'cpp')
    casm_inc = include_directories(join_paths('codec', 'common', 'loongarch'))
  elif cpu_family == 'loongarch64'
    asm_format = asm_format64
    add_project_arguments('-mlsx', '-DHAVE_LSX', '-mlasx', '-DHAVE_LASX', language: 'c')
    add_project_arguments('-DHAVE_LSX', '-DHAVE_LASX', language: 'cpp')
    casm_inc = include_directories(join_paths('codec', 'common', 'loongarch'))
  elif cpu_family == 'riscv64'
    # We dont't have riscv64-specific optimization for now.
    asm_format = asm_format64
  else
    error('FIXME: unhandled CPU family @0@ for @1@'.format(cpu_family, system))
  endif

  if ['ios', 'darwin', 'android'].contains(system)
    cpp_lib = '-lc++'
  endif
elif system == 'windows'
  if cpu_family == 'x86'
    asm_format = 'win32'
    asm_args += ['-DPREFIX', '-DX86_32']
    asm_inc = join_paths(meson.current_source_dir(), 'codec', 'common', 'x86', '')
  elif cpu_family == 'x86_64'
    asm_format = 'win64'
    asm_args += ['-DWIN64']
    asm_inc = join_paths(meson.current_source_dir(), 'codec', 'common', 'x86', '')
  elif cpu_family == 'arm'
    if cpp.get_argument_syntax() == 'msvc'
      asm_format = 'armasm'
      asm_args += ['-nologo', '-DHAVE_NEON', '-ignore', '4509']
      asm_cmds = ['armasm']
    else
      asm_format = 'clang'
      asm_args += ['-DHAVE_NEON', '-mimplicit-it=always']
      asm_cmds = cpp.cmd_array()
    endif
    asm_inc = join_paths(meson.current_source_dir(), 'codec', 'common', 'arm', '')
  elif cpu_family == 'aarch64'
    asm_format = 'armasm'
    asm_args += ['-nologo', '-DHAVE_NEON_AARCH64']
    asm_inc = join_paths(meson.current_source_dir(), 'codec', 'common', 'arm64', '')
  else
    error('FIXME: unhandled CPU family @0@ for Windows'.format(cpu_family))
  endif
else
  error('FIXME: Unhandled system @0@'.format(system))
endif

use_asm_gen = false
if cpu_family in ['x86', 'x86_64']
  nasm = find_program('nasm')

  use_asm_gen = true
  asm_gen = generator(nasm,
    output : '@BASENAME@.o',
    arguments : [
      '-f', asm_format,
      '-i', asm_inc,
      '@INPUT@',
      '-o', '@OUTPUT@'] + asm_args)
elif system == 'windows'
  if  cpu_family == 'arm'
    # For ARM, gas-preprocessor is needed for converting the asm to be
    # buildable as thumb even with Clang.
    use_asm_gen = true
    gasprep = find_program('gas-preprocessor.pl')
    asm_gen = generator(gasprep,
      output : '@BASENAME@.obj',
      arguments : [
        '-as-type', asm_format,
        '-force-thumb',
        '--'
        ] + asm_cmds + [
        '-I' + asm_inc] + asm_args + [
        '@INPUT@',
        '-c', '-o', '@OUTPUT@'])
  elif cpu_family == 'aarch64'
    # For ARM64, Clang can build the assembly as-is without needing to use
    # either gas-preprocessor or armasm64.
    if cpp.get_argument_syntax() == 'msvc' and (cpp.get_id() != 'clang-cl' or meson.version().version_compare('<0.58.0'))
      use_asm_gen = true
      gasprep = find_program('gas-preprocessor.pl')
      asm_gen = generator(gasprep,
        output : '@BASENAME@.obj',
        arguments : [
          '-as-type', asm_format,
          '-arch', 'aarch64',
          '--',
          'armasm64',
          '-I' + asm_inc] + asm_args + [
          '@INPUT@',
          '-c', '-o', '@OUTPUT@'])
    endif
  else
    # Windows only supports x86, x86_64, arm, arm64
    error('unreachable code')
  endif
endif

subdir ('codec')
subdir ('test')

libopenh264 = library('openh264',
  link_whole: [libcommon, libprocessing, libencoder, libdecoder],
  install: true,
  soversion: major_version,
  version: meson.project_version(),
  vs_module_defs: 'openh264.def',
  dependencies: deps)

openh264_dep = declare_dependency(
  link_with: libopenh264,
  include_directories: include_directories('codec/api'),
  dependencies: deps)

pkg = import('pkgconfig')
pkg.generate(libopenh264,
  description: 'OpenH264 is a codec library which supports H.264 encoding ' +
               'and decoding. It is suitable for use in real time ' +
               'applications such as WebRTC.',
)
