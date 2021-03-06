#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>

#define VERSION "1.3" 

#define expand(size) (size * 3 / 2 + 8) 

typedef unsigned char byte;

struct {
    int force_quit;
    int expand_cell;
    int stack_size;
    size_t cell_count;
}
configurations = {
    .force_quit = false,
    .stack_size = 1024,
    .cell_count = 1024,
    .expand_cell = true,
};

#define OPT_STRING "k:c:qQdD" 
struct option OPTIONS[] = {
    {"stack-size",   required_argument, NULL, 'k'},
    {"cell",         required_argument, NULL, 'c'},
    {"force-quit",   no_argument, NULL, 'q'},
    {"silent",       no_argument, NULL, 'Q'},
    {"dynamic-cell", no_argument, NULL, 'd'},
    {"static-cell",  no_argument, NULL, 'D'},
};

int list_index(int, int *);
void mark(char *, const char *, int **);
void interpret(char *);
int main(int argc, char **);

int list_index(int i, int *list)
{
    for (int j = 0; list[j] >= 0; j++)
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

#define quit() do { free(cells); free(cond_markers); free(tag_markers); return; } while (0) 
#define current_cell cells[address]
#define pop_stack() (i = stack[--stack_count]) 

void interpret(char *code)
{
    // markers
    int *cond_markers, *tag_markers;
    mark(code, "[]", &cond_markers);
    mark(code, ":", &tag_markers);

    // if input buffer hasn't meet EOF
    bool buffer = true;
    // tag count
    int tag_count = 0;
    for (; tag_markers[tag_count++] >= 0;)
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
                    for (int j = list_index(i, cond_markers) + 1; cond_markers[j] >= 0; j++)
                        if (code[cond_markers[j]] == ']') {
                            i = cond_markers[j];
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
                if (stack_count > 0) pop_stack();
                else {
                    // record index
                    current_cell = list_index(i, tag_markers);
                    // jump to next to avoid executing
                    i = tag_markers[list_index(i, tag_markers) + 1];
                }
                break;
            case ';':
                // record current location into stack
                if (stack_count >= configurations.stack_size) quit();
                stack[stack_count++] = i;
                // jump to tag
                if (current_cell <= tag_count) i = tag_markers[current_cell];
                // if tag not found
                else if (configurations.force_quit) quit();
                // go to first character (program start)
                else i = 0;
                break;
            case '=':
                marked_cell = address;
                break;
            case '_':
                address = marked_cell;
                break;
            case '~':
                address = 0;
                break;
            case '/':
                if (stack_count > 0) pop_stack();
                else quit();
                break;
            case '?':
                current_cell = !current_cell;
                break;
        }

    quit();
}

int main(int argc, char *argv[])
{
    if (argc == 1) printf(
        "Brainfuck++ Interpreter v%s\n"
        "Command: bfpp [OPTIONS...] file1 file2 ...\n"
        "OPTIONS\n"
        "    -k --stack-size [number]\n"
        "        set stack size (1024 by default)\n"
        "    -c --cell [number]\n"
        "        set cell count (1024 by default)\n"
        "    -q --force-quit\n"
        "        stop when an error occured (enabled by default)\n"
        "    -Q --silent\n"
        "        try to continue executing when error occured\n"
        "    -d --dynamic-cell\n"
        "        expand cell when trying to goes further than last cell (enabled by default)\n"
        "    -D --static-cell\n"
        "        cell count stay same\n"
        , VERSION
    );
    else {
        while (true) {
            char c = getopt_long(argc, argv, OPT_STRING, OPTIONS, NULL);
            if (c == EOF) break;
            switch (c) {
                case 'k':
                    configurations.stack_size = strtol(optarg, NULL, 10);
                    break;
                case 'c':
                    configurations.cell_count = strtol(optarg, NULL, 10);
                    break;
                case 'q':
                    configurations.force_quit = true;
                    break;
                case 'Q':
                    configurations.force_quit = false;
                    break;
                case 'd':
                    configurations.expand_cell = true;
                    break;
                case 'D':
                    configurations.expand_cell = false;
                    break;
            }
        }

        for (int i = optind; i < argc; i++) {
            FILE *fp;
            fp = fopen(argv[i], "r");

            if (fp) {
                char *buffer = NULL;

                fseek(fp, 0, SEEK_END);
                long length = ftell(fp);
                fseek(fp, 0, SEEK_SET);
                buffer = malloc(length + 1);

                if (buffer) {
                    fread(buffer, 1, length, fp);
                    buffer[length] = '\0';
                    interpret(buffer);
                    free(buffer);
                }
                
                fclose(fp);
            }
            else printf("file not found: %s\n", argv[i]);
        }
    }

    return 0;
}
