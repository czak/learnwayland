executable(
  'sample',
  'main.c',
  'display.c',
  'buffer.c',
  'input.c',
  'window.c',
  dependencies: [
    common,
    protocols,
    wayland_client,
    wayland_egl,
    dependency('egl'),
    dependency('glesv2'),
  ],
)
