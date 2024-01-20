project('learnwayland', 'c')

add_project_arguments('-D_GNU_SOURCE', language: ['c'])

wayland = dependency('wayland-client')
pixman = dependency('pixman-1')
cairo = dependency('cairo')

protocols = declare_dependency(
  sources: [
    'protocols/xdg-shell.c',
    'protocols/xdg-decoration-unstable-v1.c',
    'protocols/single-pixel-buffer-v1.c',
    'protocols/viewporter.c',
  ],
  include_directories: ['protocols'],
)

executable('01-list-globals', '01-list-globals.c', dependencies: [wayland])
executable('02-single-pixel-with-decorations', '02-single-pixel-with-decorations.c', dependencies: [wayland, protocols])
executable('03-shm-buffer-resize-no-realloc', '03-shm-buffer-resize-no-realloc.c', dependencies: [wayland, protocols])
executable('04-shm-buffer-resize-realloc', '04-shm-buffer-resize-realloc.c', dependencies: [wayland, protocols])
executable('05-pixman', '05-pixman.c', dependencies: [wayland, pixman, protocols])
executable('06-cairo', '06-cairo.c', dependencies: [wayland, cairo, protocols])
