project('learnwayland', 'c')

add_project_arguments('-D_GNU_SOURCE', language: ['c'])

wayland_client = dependency('wayland-client')
wayland_egl = dependency('wayland-egl')

common = declare_dependency(
  sources: ['common/shm.c'],
)

protocols = declare_dependency(
  sources: [
    'protocols/xdg-shell.c',
    'protocols/xdg-decoration-unstable-v1.c',
    'protocols/single-pixel-buffer-v1.c',
    'protocols/viewporter.c',
  ],
)

subdir('animation')
subdir('app-structure')
subdir('basic-window')
subdir('cairo')
subdir('gl')
subdir('list-globals')
subdir('pixman')
subdir('shm-buffer-resize-no-realloc')
subdir('shm-buffer-resize-realloc')
subdir('single-pixel-with-decorations')
subdir('subsurfaces')
subdir('toplevel-events')
