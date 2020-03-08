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
### Flags:
- -f choose function, required and must be first
- -o set image output file
- -n run multiple times on image (not all functions support this)
- -v verbose console output
- -r run on all images in directory
### Notes:
Default write out file is out.bmp
## Functionality
### Current
- None!
### Future
- Canny edge detection
- 

## Misc.
Abstract CLI to C library for future use