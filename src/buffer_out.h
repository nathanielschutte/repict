/**
 * Holds a buffer for console output strings, dynamically concat until flush out
*/

#include <string.h>
#include <stdio.h>

#define MAX_BUFFER_SIZE 300

char buffer[MAX_BUFFER_SIZE];
int buf_ptr = 0;

/* clear buffer */
void clear_buffer() {
    buf_ptr = 0;
    buffer[MAX_BUFFER_SIZE - 1] = '\0';
}

/* print buffer and clear */
void print_buffer() {
    buffer[buf_ptr] = '\0';
    printf(buffer);
    printf("\n");
    clear_buffer();
}

/* add str to buffer, length len */
void push_buffer(const char *str, size_t len) {
    if(buf_ptr + len >= MAX_BUFFER_SIZE) {
        return;
    }
    strcpy(buffer + buf_ptr, str);
    buf_ptr += len;
}

char *charPtr(int idx) { return &buffer[idx]; }