project('determinze', ['c', 'cpp'])

add_project_arguments(
  '-Wall',
  '-Wextra',
  '-Wno-unused-parameter',
  '-std=c++20',
  language: 'cpp',
)

add_global_link_arguments(
  '-static-libstdc++',
  '-static-libgcc',

  language: ['c', 'cpp'],
)

zydis_lib = static_library(
  'zydis',
  include_directories: ['vendor/zydis'],
  sources: [
    'vendor/zydis/Zydis.c',
  ],
)
zydis = declare_dependency(
  include_directories: ['vendor/zydis'],
  link_whole: zydis_lib
)

catch2 = declare_dependency(
  include_directories: ['vendor/catch2']
)

determinize_lib = static_library(
  'determinize',
  dependencies: [zydis],
  include_directories: ['include'],
  sources: [
    'src/determinize.cpp',
    'src/replacements.cpp',
    'src/thunk.cpp',
  ],
)

determinize = declare_dependency(
  include_directories: ['include'],
  link_with: determinize_lib,
)

determinize_test = executable(
  'determinize_test',
  dependencies: [determinize, catch2, zydis],
  sources: [
    'test/catch.cpp',
    'test/branch/test.cpp',
    'test/branch/test.S',
    'test/integration/test.cpp',
    'test/integration/test.S',
    'test/rcpps/test.cpp',
    'test/rcpps/test.S',
    'test/rsqrtps/test.cpp',
    'test/rsqrtps/test.S',
    'test/rsqrtss/test.cpp',
    'test/rsqrtss/test.S',
    'test/smoke/test.cpp',
    'test/smoke/test.s',
  ],
)
