project('learnwayland', 'c')

add_project_arguments('-D_GNU_SOURCE', language: ['c'])

wayland = dependency('wayland-client')

common = declare_dependency(
  sources: ['common/shm.c'],
  include_directories: ['common'],
)

protocols = declare_dependency(
  sources: [
    'protocols/xdg-shell.c',
    'protocols/xdg-decoration-unstable-v1.c',
    'protocols/single-pixel-buffer-v1.c',
    'protocols/viewporter.c',
  ],
  include_directories: ['protocols'],
)

executable('list-globals', 'list-globals.c', dependencies: [wayland])
executable('decorations', 'decorations.c', dependencies: [wayland, protocols, common])
