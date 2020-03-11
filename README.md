# Repict
Image editing function library

## Author
Nathaniel Schutte

## Help
### Build (MinGW GCC):
Does not require any includes, just all files in src folder... makefile available:
```
make
```
### How to use:
```
repict <image.bmp> -f <function> <...>
repict <image.bmp> -f <function> <...> -o <image_out.bmp>
```
### For filetype conversion only:
```
repict <image.bmp> -o <image.png>
```
### For complete help:
```
repict help
```
### Notes:
- Default write out file is out/output.png (with no -o flag)
- Output can be any supported format
- Each function takes a different set of arguments (each usage in 'help')
### Flags:
- -f choose function, required and must be first
- -o set image output file
- -n run multiple times on image (not all functions support this)
- -v verbose console output
- -r run on all images in directory (not supported yet)

## Functionality
### Current
- File format conversion
- B&W filter
- Gaussian blur
- Average blur
- Custom kernel input and convolution
### Future
- Canny edge detection
- Load kernel from .txt file
- Contrast normalization
- Luminance filter
- Bump maps

## Misc....
- Abstract CLI to C library for future use
- Make a basic GUI to run these filters on images and see realtime results
