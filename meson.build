project('markov', 'c')

cc = meson.get_compiler('c')

markov_dependencies = [
]

include = [
  include_directories('.'),
  include_directories('external/cvector'),
  include_directories('external/stringbuilder'),
  include_directories('external/ok_lib'),
  include_directories('external/pcg'),
]

source_files = [
  'source/main.c',
  'source/markov.c',
]

subdir ('external/pcg')

markov_binary = executable (
  'markov',
  source_files,
  include_directories : include,
  dependencies : [markov_dependencies],
)
