# Repict
Image editing function library

## Author
Nathaniel Schutte

## Help
### Build (MinGW GCC):
```
make
```
### How to use:
```
repict <image.bmp> -f <function> <...>
repict <image.bmp> -f <function> <...> -o <image_out.bmp>
```
### For complete help:
```
repict help
```
### Notes:
- Default write out file is out.bmp
- Each function takes a different set of arguments
### Flags:
- -f choose function, required and must be first
- -o set image output file
- -n run multiple times on image (not all functions support this)
- -v verbose console output
- -r run on all images in directory (not supported yet)
## Functionality
### Current
- File format conversion
### Future
- Canny edge detection
- B&W filter
- Gaussian blur

## Misc.
- Abstract CLI to C library for future use
- Make a basic GUI to run these filters on images and see realtime results