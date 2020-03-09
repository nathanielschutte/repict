/*
 * Repict main file
*/


// =========== TODO LIST ===========

// TODO: use STB to convert PNG (and other formats) to BMP
// repict -c <image.png>
// outputs image.bmp for use with bmpio (just to make sure my own read/write works)

// TODO: just accept multiple formats and convert to using STB entirely...

// TODO: open a window with output image

#include "repict_cli.h"
#include "buffer_out.h"
#include "repict.c"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"


/* Just return data */
pixel_t *default_op(pixel_t *data, int argc, char **argv) {
    return data;
}

/* Resize to width: args[0] height: args[1] */
pixel_t *resize_op(pixel_t *data, int argc, char **argv) {
    return data;
}

/* Print help menu
    doesn't really require this method signature but it has to align */
void print_help() {
    printf("\nFunctions:\n");
    for (unsigned int i = 0; i < MAX_FUNCTIONS; i++) {
        printf(" - ");
        printf(functions[i].name);
        printf(":  \t");
        print_usage_f(functions[i], true);
    }
    printf("\nUse -o <out.png> to set custom output file (use supported extensions)\n");
    printf("Use -v to turn on verbose feedback\n");
    printf("Use -n to set number of times function applied\n\n");
    printf("Supported extensions:\n");
    for (unsigned int i = 0; i < MAX_FORMATS; i++) {
        if (formats[i].format == NONE) {
            continue;
        }
        printf(" - ");
        printf(formats[i].ext);
        printf("\n");
    }
}

/* Get file format from input path, return null if not supported */
FORMAT match_file_format(char *file) {
    file_type = "error";
    if (file == NULL) {
        return NONE;
    }

    char *ext = strchr(file, (int) '.');
    if (ext == NULL) {
        printf("repict: enter a valid pathname to image\n");
        return NONE;
    }

    for (unsigned int i = 0; i < MAX_FORMATS; i++) {
        if (strcmp(++ext, formats[i].ext) == 0) {
            file_type = formats[i].ext;
            return formats[i].format; // format match
        }
    }

    return NONE; // no match
}

/* Get function from input */
function_t *match_function(char *in) {
    if (in == NULL) {
        return NULL;
    }
    for (unsigned int i = 0; i < MAX_FUNCTIONS; i++) {
        if (strcmp(in, functions[i].name) == 0) {
            return &functions[i];
        }
    }
    return NULL;
}

/* Handle -f function select and args, call function */
bool handle_function(const int argc, const char **argv) {
    char *func_name = argv[0];
    function_t *func = match_function(func_name);
    if (func == NULL) {
        printf("repict: no such function\n");
        return false;
    }
    function = *func;
    if (argc - 1 >= func->arg_min) {
        int a = 1;
        f_argc = 0;
        f_argv = malloc(func->arg_max * sizeof(char *));

        // accumulate args: within bounds, within max, not the next flag
        while (a < argc && a - 1 < func->arg_max && argv[a][0] != '-') {
            f_argv[a - 1] = argv[a];
            a++;
        }
        f_argc = a - 1;

        if (f_argc < func->arg_min) {
            printf("repict: too few arguments given to specified function\n");
            usage_req = true;
            return false;
        }
        else if (f_argc > func->arg_max) {
            printf("repict: too many arguments given to specified function\n");
            usage_req = true;
            return false;
        }

        function_def = true;
        return true;

    }
    printf("repict: too few arguments given to specified function\n");
    usage_req = true;
    return false;
}

/* Open file for use, return false on failure */
bool open_file(char *file, FORMAT format) {
    if (file == NULL) {
        return false;
    }
    
    pixel_t *pi;
    switch (format) {

        // use bmpio to load BMP from file
        case F_BMP:
            pi = load_bmp(file, bmp_ih);
            if (pi == NULL) {
                return false;
            }
            pixels = pi;
            width = bmp_ih->width;
            height = bmp_ih->height;

        break;
        case F_PNG:
            pixels = stbi_load(file, &width, &height, &bpp, CHANNELS);
        break;
        default:
        return false;
    }
    return true;
}

bool write_file(char *file, FORMAT format) {
    if (file == NULL) {
        return false;
    }
    switch (format) {
        case F_BMP:

            // use bmpio for now
            if (! save_bmp(file, bmp_ih, pixels_out)) {
                printf("repict: could not save BMP file\n");
                return false;
            }

            //stbi_write_bmp(file, width, height, CHANNELS, pixels_out);
        break;

        case F_PNG:
            stbi_write_png(file, width, height, CHANNELS, pixels_out, width*CHANNELS);
        break;

        default:
        return false;
    }
    return true;
}

/* Handle flags */
bool handle_flags(const int argc, const char **argv) {
    for (unsigned int i = 2; i < argc; i++) {
        char *arg = argv[i];
        if(arg[0] == '-') {
            const char flag = arg[1];
            switch (flag) {
                case 'f': 
                if (argc >= i) {
                    if (! handle_function(argc - i - 1, argv + i + 1)) {
                        return false;
                    }
                }
                break;

                case 'v':
                verbose = true;
                break;

                case 'o':
                if (argc >= i) {
                    file_out = argv[i + 1];
                }
                else {
                    printf("repict: no output file specified\n");
                }
                break;
            }
        }
    }
    return true;

    // no flags
}


/* Main */
int main(const int argc, const char** argv) {
    verbose = false;
    function_def = false;
    usage_req = false;
    file_out = DEFAULT_OUT_FILE;
    clear_buffer();

    // check for help
    if(argc > 1 && strcmp(argv[1], "help") == 0) {
        print_help();
        return 0;
    }
    
    // get input file or fail
    if (argc < 4) {
        printf("repict: not enough arguments provided\n");
        print_usage(true);
        return 0;
    }

    file_in = argv[1];

    // special case: use r to use default out file as input
    FORMAT format;
    if (strcmp(file_in, "r") == 0) {
        file_in = DEFAULT_OUT_FILE;
        format = DEFAULT_OUT_FORMAT.format;
        file_type = DEFAULT_OUT_FORMAT.ext;
    }
    else {

        // check input file format
        format = match_file_format(file_in);
        if (format == NONE) {
            printf("repict: error reading file format of input\n");
            return 0;
        }
    }

    // open file, store data in pixels
    if (! open_file(file_in, format)) {
        printf("repict: failure opening file\n");
        return 0;
    }

    // go through all flags
    if (! handle_flags(argc, argv)) {
        // errors handled within
        if (usage_req) {
            print_usage_f(function, false);
        }
        return 0;
    }

    print_verbose("Input filetype:", file_type);

    if (! function_def) {
        printf("repict: no function specification provided\n");
        print_usage(false);
        return 0;
    }

    // call function exec (TODO: implement multiple calls)
    pixels_out = function.exec(pixels, f_argc, f_argv);
    free(f_argv);

    // write output to output file with format specification
    FORMAT format_out = match_file_format(file_out);
    if (format_out == NONE) {
        printf("repict: error reading file format of output\n");
        return 0;
    }

    print_verbose("Output filetype:", file_type);

    if (! write_file(file_out, format_out)) {
        printf("repict: failure writing output file\n");
        return 0;
    }

    // free image memory, clean
    if (format == F_PNG) {
        stbi_image_free(pixels);
    }
}


/* Print a verbose only message */
void print_verbose(const char *lbl, const char *msg) {
    if(verbose) {
        printf(lbl);
        printf(" ");
        printf(msg);
        printf("\n");
    }
}

/* Print commandline usage of repict function */
void print_usage_f(function_t f, bool omit_out) {
    clear_buffer();
    push_buffer("Usage:  ", 8);
    push_buffer(DEFAULT_ARG, strlen(DEFAULT_ARG));
    push_buffer(" ", 1);
    push_buffer("-f ", 3);
    push_buffer(f.name, strlen(f.name));
    push_buffer(" ", 1);
    push_buffer(f.usage, strlen(f.usage));
    if (! omit_out) {
        push_buffer(" ", 1);
        push_buffer(DEFAULT_OUT, strlen(DEFAULT_OUT));
    }
    print_buffer();
}

/* Print commandline usage of repict generally */
void print_usage(bool omit_out) {
    clear_buffer();
    push_buffer("Usage:  ", 8);
    push_buffer(DEFAULT_ARG, strlen(DEFAULT_ARG));
    push_buffer(" ", 1);
    push_buffer(DEFAULT_USAGE, strlen(DEFAULT_USAGE));
    if (! omit_out) {
        push_buffer(" ", 1);
        push_buffer(DEFAULT_OUT, strlen(DEFAULT_OUT));
    }
    print_buffer();
}



// for displaying result image in a Windows window (unused)

#ifdef DO_WINDOWS_GRAPHICS
const char g_szClassName[] = "myWindowClass";

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_CLOSE:
            DestroyWindow(hwnd);
        break;
        case WM_DESTROY:
            PostQuitMessage(0);
        break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASSEX wc;
    HWND hwnd;
    MSG Msg;

    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = 0;
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = g_szClassName;
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

    if(!RegisterClassEx(&wc))
    {
        MessageBox(NULL, "Windows failure", "Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // Step 2: Creating the Window
    hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        g_szClassName,
        "repict",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 240, 120,
        NULL, NULL, hInstance, NULL);

    if(hwnd == NULL)
    {
        MessageBox(NULL, "Windows failure", "Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    while(GetMessage(&Msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    return Msg.wParam;
}

#endif