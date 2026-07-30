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
cd koboldkeys && cmake -B build -DCMAKE_BUILD_TYPE=Release &&
cmake --build build
```

The compiled executable will be under "build/koboldkeys".

### Configuring

An example `config.toml` and `style.css` can be found in this repo. Koboldkeys searches `XDG_CONFIG_HOME/koboldkeys/` for these two files. Which defaults to `$HOME/.config/koboldkeys` if unset.

A path can be explicitly specified through the `-c` flag if desired
