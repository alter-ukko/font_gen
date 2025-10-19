## font_gen

`font_gen` is a font generator for clover. It's basically a wrapper around [stb_truetype](https://github.com/nothings/stb) that outputs both a png image of the font and a C file that contains a clover font definition.

```shell
Usage: font_gen [options] ttf_filename

Generates a font texture and clover font def from a TTF file.

    -h, --help            show this help message and exit

Options
    -s, --size=<flt>      pixel height of font
    -a, --ascent=<int>    find a font size closest to this ascent
    -x, --x=<int>         width of the atlas
    -y, --y=<int>         height of the atlas
    -i, --image           output png binary data in c file
```

## building

Clone the repo. `cd` into the project folder. Run: 

```shell
mkdir build && cd build && cmake .. && make
```
