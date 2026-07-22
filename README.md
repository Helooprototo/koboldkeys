# Koboldkeys

A wayland keyboard overlay that works through reading the input device events from /dev/input

## Building

### Dependencies

- gtk-layer-shell
- gtk-3.0
- tomlplusplus
- xkbcommon
- pkgconfig

### Compiling

```sh
git clone https://github.com/Helooprototo/koboldkeys.git &&
cd koboldkeys/src &&
mkdir build &&
cd build &&
cmake .. &&
make;
```
