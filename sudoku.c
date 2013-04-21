#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct {
    uint8_t values[81];        // Contains all square values. 0 means unknown
    uint16_t options[81];      // Bit-field with possible options for square.
                               // 0 means value is known.
    uint8_t options_count[81]; // Options count for square. 0 means value is known.
    uint8_t unit_count[9];     // Unknown square count in unit
    uint16_t unit_options[9];  // Bit-field with value not defined in unit
    uint8_t modified;          // Used by constraint propagation
    uint8_t count;             // Unknown squares left
} board_t;

int board_set(board_t *board, uint8_t i, uint8_t j, uint8_t v);

const static uint16_t o_mask = (1 << 9) - 1;
const static uint16_t options[10] = {
    0, 1 << 0, 1 << 1, 1 << 2, 1 << 3, 1 << 4, 1 << 5, 1 << 6, 1 << 7, 1 << 8
};

void board_init(board_t *board)
{
    int i;
    if(board) {
        for(i = 0; i < 81; i++) {
            board->values[i] = 0;
            board->options[i] = o_mask;
            board->options_count[i] = 9;
        }
        for(i = 0; i < 9; i++) {
            board->unit_count[i] = 0;
            board->unit_options[i] = o_mask;
        }
        board->modified = 0;
        board->count = 81;
    }
}

void board_copy(board_t *dst, board_t *src)
{
    if(dst && src) {
        memcpy(dst, src, sizeof(board_t));
    }
}

inline uint8_t option_to_value(uint16_t value) {
    switch(value) {
        case (1 << 0): return 1;
        case (1 << 1): return 2;
        case (1 << 2): return 3;
        case (1 << 3): return 4;
        case (1 << 4): return 5;
        case (1 << 5): return 6;
        case (1 << 6): return 7;
        case (1 << 7): return 8;
        case (1 << 8): return 9;
        default:
            return 0;
    }
}

inline uint16_t value_to_option(uint8_t option)
{
    return (option > 0 && option < 10) ? options[option] : o_mask;
}


inline uint8_t board_offset(uint8_t i, uint8_t j)
{
    return j * 9 + i;
}

inline int test_cell(board_t *board, uint8_t x, uint8_t y, uint8_t v, uint16_t opt)
{
    uint8_t offset = board_offset(x, y);
    if(board->values[offset] == v) {
        return 1;
    } else if(board->values[offset] == 0) {
        if((board->options[offset] & opt)) {
            uint16_t o = board->options[offset];
            board->options[offset] &= (o_mask ^ opt);
            board->options_count[offset]--;
            if(board->options_count[offset] == 1) {
                 uint8_t val = option_to_value(board->options[offset]);
                 if(board_set(board, x, y, val)) {
                     return 1;
                 }
            }
        }
    }
    return 0;
}

int check_unit_left(board_t *board, uint8_t u_offset)
{
    uint8_t x, y, l;
    uint8_t ui = (u_offset % 3) * 3;
    uint8_t uj = (u_offset / 3) * 3;

    for (l = 1; l < 10; l++) {
        uint16_t m = value_to_option(l);
        uint8_t lx, ly, l_count = 0;
        if((m & board->unit_options[u_offset]) == 0) {
            continue;
        }
        for (y = uj; y < (uj + 3); y++) {
            for (x = ui; x < (ui + 3); x++) {
                uint8_t offset = board_offset(x, y);
                if(board->values[offset] == 0) {
                    if(board->options[offset] & m) {
                        lx = x;
                        ly = y;
                        l_count++;
                    }
                }
            }
        }
        if(l_count == 1) {
            if(board_set(board, lx, ly, l)) {
                return 1;
            }
        }
    }

    return 0;
}

int check_unit(board_t *board, uint8_t i, uint8_t j, uint8_t v, uint16_t opt)
{
    uint8_t x, y;

    // Compute (ui, uj): the unit's upper-left coordinate
    uint8_t ui = (i / 3) * 3;
    uint8_t uj = (j / 3) * 3;

    // Eliminate 
    for (y = uj; y < (uj + 3); y++) {
        for (x = ui; x < (ui + 3); x++) {
            if(!(x == i && y == j)) {
                if(test_cell(board, x, y, v, opt)) {
                    return 1;
                }
            }
        }
    }

    return 0;
}

int check_hline(board_t *board, uint8_t i, uint8_t j, uint8_t v, uint16_t opt)
{
    uint8_t x;
    for (x = 0; x < 9; x++) {
        if(x != i) {
            if(test_cell(board, x, j, v, opt)) {
                return 1;
            }
        }
    }
    return 0;
}

int check_vline(board_t *board, uint8_t i, uint8_t j, uint8_t v, uint16_t opt)
{
    uint8_t y;
    for (y = 0; y < 9; y++) {
        if(y != j) {
            if(test_cell(board, i, y, v, opt)) {
                return 1;
            }
        }
    }
    return 0;
}


int board_set(board_t *board, uint8_t i, uint8_t j, uint8_t v)
{
    uint8_t offset = board_offset(i, j);
    uint8_t u_offset = (i / 3) + 3 * (j / 3);
    uint16_t opt = value_to_option(v);

    board->values[offset] = v;
    board->options[offset] = 0;
    board->options_count[offset] = 0;

    board->unit_count[u_offset]++;
    board->unit_options[u_offset] &= (o_mask ^ opt);

    board->modified = 1;
    board->count--;

    if(check_unit(board, i, j, v, opt) ||
       check_hline(board, i, j, v, opt) ||
       check_vline(board, i, j, v, opt)) {
        return 1;
    }
    return 0;
}

int board_check_single(board_t *board)
{
    while(board->modified) {
        uint8_t u_offset;
        board->modified = 0;
        for(u_offset = 0; u_offset < 9; u_offset++) {
            if(check_unit_left(board, u_offset)) {
                return 1;
            }
        }
    }
    return 0;
}


void board_load(board_t *board, char *line)
{
    int i, j;

    for(j = 0; j < 9; j++) {
        for(i = 0; i < 9; i++) {
            uint8_t offset = board_offset(i, j);
            char c = line[offset];
            if(c >= '1' && c <= '9') {
                board_set(board, i, j, c - '0');
            }
        }
    }
}

int board_solve(board_t *board)
{
    if(board->count == 0) {
        return 0;
    } else {
        board_t current;

        // Identify the maximum number of indentified
        // squares per unit.
        uint8_t u, offset, o, u_max = 0, o_min = 9;
        for (u = 0; u < 9; u++) {
            if(board->unit_count[u] > u_max && board->unit_count[u] < 9) {
                u_max = board->unit_count[u];
            }
        }

        // Find the square with the minimum number of possible options
        // within a unit with the maximum number of identified squares.
        uint8_t lx, ly;
        for (u = 0; u < 9; u++) {
            if(board->unit_count[u] == u_max) {
                uint8_t ui = (u % 3) * 3;
                uint8_t uj = (u / 3) * 3;
                uint8_t x, y;
                for (y = uj; y < (uj + 3); y++) {
                    for (x = ui; x < (ui + 3); x++) {
                        uint8_t o = board_offset(x, y);
                        if(board->values[o] == 0 &&
                           board->options_count[o] < o_min ) {
                            o_min = board->options_count[o];
                            lx = x;
                            ly = y;
                            offset = o;
                        }
                    }
                }
            }
        }

        uint8_t l;
        for (l = 1; l < 10; l++) {
            uint16_t m = value_to_option(l);
            if((m & board->options[offset])) {
                board_copy(&current, board);
                if(board_set(&current, lx, ly, l) == 0 &&
                   board_check_single(&current) == 0 &&
                   board_solve(&current) == 0) {
                    board_copy(board, &current);
                    return 0;
                }
            }
        }
    }
    return 1;
}

void board_dump(FILE *f, board_t *board)
{
    static char display[] = " 123456789";
    int i, j;
    for(j = 0; j < 9; j++) {
        for(i = 0; i < 9; i++) {
            uint8_t val = board->values[board_offset(i, j)];
            fprintf(f, "%c ", display[val]);
            if((i == 2)||(i == 5)) {
                fprintf(f, "| ");
            }
        }
#ifdef DEBUG
        fprintf(f, "  ");
        for(i = 0; i < 9; i++) {
            uint16_t opt = board->options[board_offset(i, j)];
            fprintf(f, "%X%X%X ", (opt >> 8) % 16, (opt >> 4) % 16, opt % 16);
        }

        fprintf(f, "  ");
        for(i = 0; i < 9; i++) {
            uint8_t cnt = board->options_count[board_offset(i, j)];
            fprintf(f, "%d ", cnt);
        }
#endif
        fprintf(f, "\n");
        if((j == 2)||(j == 5)) {
            fprintf(f, "------+-------+------\n");
        }
    }
    fprintf(f, "\n");
}


int main(int argc, char **argv)
{
    FILE *f;
    board_t board;
    char *line = malloc(128);
    int verbose = 0;

    if(argc > 1 && strcmp(argv[1], "-v") == 0) {
        verbose = 1;
    }

    if((argc < 2) || (verbose && argc < 3)) {
        fprintf(stderr, "Syntax:\n\tsudooku [-v] <file>\n\n");
        return -1;
    }


    if((f = fopen(argv[verbose ? 2 : 1], "r")) == NULL) {
        fprintf(stderr, "Error opening file '%s'\n", argv[verbose ? 2 : 1]);
        return -1;
    }

    while (!feof(f) && (fgets(line, 128, f) != NULL)) {
        if(strlen(line) == 82) {
            board_init(&board);
            board_load(&board, line);
            if(verbose) {
                board_dump(stdout, &board);
            }
            board_solve(&board);
            if(verbose) {
                board_dump(stdout, &board);
            }
        }
    }

    fclose(f);
    free(line);
    exit(0);
}
