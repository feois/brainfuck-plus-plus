#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#define expand(size) (size * 3 / 2 + 8) 

int list_index(int i, int *list)
{
    for (int j = 0; list[j] > 0; j++)
        if (i == list[j]) return j;
    return -1;
}

// mark all locations of any char of the focus into a list
void mark(char *string, const char *focus, int **markers)
{
    size_t size = 8;
    size_t count = 0;
    // initialise the marker list of size 8
    *markers = malloc(sizeof(int) * size);
    
    // if memory allocated
    if (*markers)
        // loop string
        for (int i = 0; string[i]; i++)
            // loop focus to test if any of the focus equal to current char
            for (int j = 0; focus[j]; j++)
                //if current char equals to one of the focus
                if (string[i] == focus[j]) {
                    // if marker list is not big enough
                    if (count + 1 >= size) {
                        size_t new_size = expand(size);
                        int *ptr = realloc(*markers, new_size);
                        if (ptr == NULL) {
                            free(*markers);
                            *markers = NULL;
                            return;
                        }

                        *markers = ptr;
                        size = new_size;
                    }

                    // add marker of current location
                    (*markers)[count++] = i;
                    break;
                }
    
    // set last marker as -1 (list terminator)
    (*markers)[count] = -1;
}

typedef unsigned char byte;

#define quit() do { free(cells); free(cond_markers); free(tag_markers); return; } while (0) 
#define current_cell cells[address]

struct {
    bool force_quit;
    int stack_size;
    size_t cell_count;
    bool expand_cell;
}
configurations = {
    .force_quit = false,
    .stack_size = 1024,
    .cell_count = 1024,
    .expand_cell = true,
};

void interpret(char *code) {
    // markers
    int *cond_markers, *tag_markers;
    mark(code, "[]", &cond_markers);
    mark(code, ":", &tag_markers);

    // if input buffer hasn't meet EOF
    bool buffer = true;
    // tag count
    int tag_count = 0;
    for (; tag_markers[tag_count] > 0; tag_count++)
        ;

    // cell being pointed at
    size_t address = 0;
    // cell size (may be expanded)
    size_t cell_size = configurations.cell_count;
    // stacks (record locations before jumping)
    int stack[configurations.stack_size];
    int stack_count = 0;
    // cell that is marked with '='
    int marked_cell = 0;
    // all cells
    byte *cells = malloc(sizeof(byte) * cell_size);

    // loop code
    for (int i = 0; code[i]; i++)
        switch (code[i]) {
            case '<':
                // if address is zero, jump to last cell
                if (!address) address = cell_size;
                address--;
                break;
            case '>':
                address++;
                // if address is larger than cell size
                if (address >= cell_size) {
                    if (!configurations.expand_cell) address = 0;
                    else {
                        size_t new_size = expand(cell_size);
                        byte *ptr = realloc(cells, new_size);
                        if (ptr) {
                            cells = ptr;
                            cell_size = new_size;
                        }
                        // force quit if memory allocation failed
                        else if (configurations.force_quit)
                            quit();
                        else
                            address = 0;
                    }
                }
                break;
            case '+':
                current_cell++;
                break;
            case '-':
                current_cell--;
                break;
            case '.':
                putchar(current_cell);
                break;
            case ',':
                current_cell = 0;
                // if input buffer hasn't met EOF
                if (buffer) {
                    int c = getchar();
                    if (c == EOF) {
                        buffer = false;
                        // quit if EOF
                        if (configurations.force_quit)
                            quit();
                    }
                    else current_cell = c;
                }
                break;
            case '[':
                if (!current_cell)
                    // search all condition markers that is behind this
                    for (int j = list_index(i, cond_markers) + 1; cond_markers[j] > 0; j++)
                        if (code[cond_markers[j]] == ']') {
                            i = cond_markers[j];
                            printf("LOLOLOL%d\n", i);
                            break;
                        }
                break;
            case ']':
                if (current_cell)
                    // search all condition markers that is before this
                    for (int j = list_index(i, cond_markers) - 1; j >= 0; j--)
                        if (code[cond_markers[j]] == '[') {
                            i = cond_markers[j];
                            break;
                        }
                break;
            case ':':
                // jump back to previous stack
                if (stack_count > 0) i = stack[stack_count--];
                else {
                    // record index
                    current_cell = list_index(i, tag_markers);
                    // jump to next to avoid executing
                    i = tag_markers[list_index(i, tag_markers) + 1];
                }
                break;
            case ';':
                // record current location into stack
                stack[stack_count++] = i;
                // jump to tag (if tag isn't found, jump to 0 which is program start)
                i = current_cell > tag_count ? 0 : tag_markers[current_cell];
                break;
            case '=':
                marked_cell = address;
                address = 0;
                break;
            case '_':
                address = marked_cell;
                break;
        }

    quit();
}

int main(int argc, char *argv[])
{
    interpret("your program here");

    return 0;
}