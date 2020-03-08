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
pixel_t *print_help(pixel_t *data, int argc, char **argv) {
    printf("HELP: %u\n\n", argc);
}

/* Get file format from input path, return null if not supported */
FORMAT match_file_format(char *file) {
    if (file == NULL) {
        return NULL;
    }

    char *ext = strchr(file, (int) '.');
    if (ext == NULL) {
        fprintf(stderr, "Error: enter a valid pathname to image");
        return NULL;
    }

    for (unsigned int i = 0; i < MAX_FORMATS; i++) {
        if (strcmp(++ext, formats[i].ext)) {
            return formats[i].format; // format match
        }
    }

    return NULL; // no match
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
        fprintf(stderr, "repict: no such function\n");
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
            fprintf(stderr, "repict: too few arguments given to specified function");
            usage_req = true;
            return false;
        }
        else if (f_argc > func->arg_max) {
            fprintf(stderr, "repict: too many arguments given to specified function");
            usage_req = true;
            return false;
        }

        function_def = true;
        return true;

    }
    fprintf(stderr, "repict: too few arguments given to specified function");
    usage_req = true;
    return false;
}

/* Open file for use, return false on failure */
bool open_file(char *file, FORMAT format) {
    if (file == NULL) {
        return false;
    }
    switch (format) {

        // use bmpio to load BMP from file
        case F_BMP:
            bitmap_info_header_t* bmp_ih;
            pixel_t *pi = load_bmp(file, bmp_ih);
            if (pi == NULL) {
                return false;
            }
            pixels = pi;
            width = bmp_ih->width;
            height = bmp_ih->height;

        break;
        case F_PNG:
            pixels = stbi_load(file, &width, &height, &bpp, 3);
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
            stbi_write_bmp(file, width, height, 3, pixels_out);
        break;
        case F_PNG:
            stbi_write_png(file, width, height, 3, pixels_out, width*3);
        break;
        default:
            return false;
    }
    return true;
}

/* Handle flags */
bool handle_flags(const int argc, const char **argv) {
    for (unsigned int i = 2; i < argc; i++) {
        if(argv[i][0] == '-') {
            const char flag = argv[i][1];
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
                    fprintf(stderr, "repict: no output file specified\n");
                }
                break;
            }
        }
    }
}


/* Main */
int main(const int argc, const char** argv) {
    verbose = false;
    function_def = false;
    usage_req = false;
    file_out = DEFAULT_OUT_FILE;
    char *exec = argv[0];
    clear_buffer();
    
    // get input file or fail
    if (argc < 4) {
        fprintf(stderr, "repict: not enough arguments provided\n");
        print_usage(exec, false);
        return 0;
    }
    file_in = argv[1];

    // check input file format
    FORMAT format = match_file_format(file_in);
    if (format == NULL) {
        fprintf(stderr, "repict: error reading file format of input\n");
        return 0;
    }

    // open file, store data in pixels
    if (! open_file(file_in, format)) {
        fprintf(stderr, "repict: failure opening file\n");
        return 0;
    }

    // go through all flags
    if (! handle_flags(argc, argv)) {
        // errors handled within
        if (usage_req) {
            print_usage_f(argv[0], function, false);
        }
        return 0;
    }

    if (! function_def) {
        fprintf(stderr, "repict: no function specification provided\n");
        print_usage(exec, false);
        return 0;
    }

    // call function exec (TODO: implement multiple calls)
    pixels_out = function.exec(pixels, f_argc, f_argv);
    free(f_argv);

    // write output to output file with format specification
    FORMAT format_out = match_file_format(file_out);
    if (format_out == NULL) {
        fprintf(stderr, "repict: error reading file format of output\n");
        return 0;
    }

    if (! write_file(file_out, format_out)) {

    }

    // free image memory, clean
    if (format == F_PNG) {
        stbi_image_free(pixels);
    }
}


/* Print a verbose only message */
void print_verbose(const char *msg) {
    if(verbose) {
        printf(msg);
        printf("\n");
    }
}

/* Print commandline usage of repict function */
void print_usage_f(const char *arg0, function_t f, bool omit_out) {
    printf(arg0);
    printf(" ");
    printf(DEFAULT_USAGE);
    if (! omit_out) {
        printf(DEFAULT_OUT);
    }
}

/* Print commandline usage of repict generally */
void print_usage(const char *arg0, bool omit_out) {
    clear_buffer();
    push_buffer(arg0, strlen(arg0));
    push_buffer(" ", 1);
    push_buffer(DEFAULT_USAGE, strlen(DEFAULT_USAGE));
    if (! omit_out) {
        push_buffer(DEFAULT_OUT, strlen(DEFAULT_OUT));
    }
    print_buffer();
    printf("\n");
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