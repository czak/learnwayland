This is a collection of my practice programs for Wayland client development.
Each program is more-or-less self contained and covers a single concept or protocol.

## How to build

Build with [Meson](https://mesonbuild.com/):

```sh
meson setup build
meson compile -C build
```

Then run e.g. `./build/animation/sample`.
