# Koboldkeys

A wayland keyboard overlay that works through reading the input device events from /dev/input. Made to fill a gap ive noticed in the Wayland ecosystem.

Written in C using gtk-3.0 as the ui. Built to consume as little ram as possible, with the barebones style using only 30mb on my machine.

A thanks to the showmethekeys project for inspiring this idea!

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

The main way to style koboldkeys and make it look "good" is through the `style.css` file, with gtk-3.0 css.
An overview of this css can be found [on the gtk-3.0 docs](https://docs.gtk.org/gtk3/css-overview.html)

All visible widgets can be specified through the `button` selector.

```css
button{
    color:black;
}
```

keyboard keys can be selected through the `keyboardbutton` class.

```css
.keyboardbutton{
    color:white;
}
```

Mouse keys can be selected through the `mousebutton` class.

```css
.mousebutton{
    color:orange;
}
```

Mouse wheels can be selected through the `mousewheel` class.

```css
.mousewheel{
    color:transparent;
}
```

The "Pressed" state of a key can be selected through the `:checked` state.

```css
.keyboardbutton:checked{
    background: orange;
}
```

For mouse wheels the Up and Down state can be selected through the `up` and `down` *class* respectively.

```css
.mousewheel.up{
    background:green;
}

.mousewheel.down{
    background:red;
}
```

A specific key can be selected through the key given to it in the `config.toml` file

toml:

```toml
[button.TheAButton]
sym="A"
```

css:

```css
button#TheAButton{
    background:purple;
    font: 94px "impact";
}
button#TheAButton:checked{
    background:black
}
```

An example for a "styled" configuration can be found under the "preview" folder in this repo.

A path to the folder containing both the `config.toml` and `style.css` can be explicitly specified through the `-c` flag if desired.
