# gifcolormap
Colormap management for gif image via CLI

# How to Build

The project is based on autotools. If you're on Windows, try using Cygwin/Mingw-w64 terminal so you
might be able to build this project.

* `./autogen.sh`
* `./configure`
* `make -j4`
* `sudo make install`

# How to Use

`gifcolormap` is designed to have a few more functionality related to color map.
At the moment, it has `-add-color` command to add color into colormap.

* `-add-color r,g,b` - to add a new color into colormap by appending it at the end of color map which
is starting at 256th position, then 255th and so on. You can add this command multiple times. If such
color already existed in the colormap, it will skip, and proceed to the next one. `r,g,b` are values
0-255 for red, green, and blue channel respectively. Ex. `gifcolormap -add-color 123,123,123 -add-color 200,200,200 input.gif output.gif`

# License
[MIT](https://github.com/abzico/gifcolormap/blob/master/LICENSE), [Angry Baozi](https://abzi.co)
