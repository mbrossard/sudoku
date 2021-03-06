#include <strings.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct {
    uint8_t values[81];        // Contains all square values. 0 means unknown
    uint16_t options[81];      // Bit-field with possible options for square.
                               // 0 means value is known.
    uint8_t options_count[81]; // Options count for square. 0 means value known
    uint8_t unit_count[9];     // Unknown square count in unit
    uint8_t hline_count[9];    // Unknown square count in hline
    uint8_t vline_count[9];    // Unknown square count in vline
    uint16_t unit_options[9];  // Bit-field with value not defined in unit
    uint16_t hline_options[9]; // Bit-field with value not defined in hline
    uint16_t vline_options[9]; // Bit-field with value not defined in vline
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
            board->hline_count[i] = 0;
            board->hline_options[i] = o_mask;
            board->vline_count[i] = 0;
            board->vline_options[i] = o_mask;
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
    int rv = 0;
    uint8_t offset = board_offset(x, y);
    if(board->values[offset] == v) {
        rv = 1;
    } else if(board->values[offset] == 0) {
        if((board->options[offset] & opt)) {
            board->options[offset] &= (o_mask ^ opt);
            board->options_count[offset]--;
            if(board->options_count[offset] == 1) {
                 uint8_t val = option_to_value(board->options[offset]);
                 if(board_set(board, x, y, val)) {
                     rv = 1;
                 }
            }
        }
    }
    return rv;
}

// The function will check if there a single cell for a given value in
// all units, horizontal and vertical lines. This function breaks on
// finding a solution.
int board_check_single_step(board_t *board)
{
    uint8_t x, y, u, l;

    ////////////////////////////////////////////////////////////////////
    // Check all units to see if one cell value can be determined
    // because it's the only one left with a given option.
    for(u = 0; u < 9; u++) {
        // Checks if the unit is already full
        if(board->unit_count[u] == 9) {
            continue;
        }

        uint8_t ui = (u % 3) * 3, uj = (u / 3) * 3;
        for (l = 1; l < 10; l++) { // Iterate on possible values
            uint8_t lx = 0, ly = 0, lc = 0;
            uint16_t m = value_to_option(l);

            // Skip if the value is already used in the unit
            if ((m & board->unit_options[u]) == 0) {
                continue;
            }

            // Find if there's only one possible cell
            for (y = uj; y < (uj + 3) && lc < 2; y++) {
                for (x = ui; x < (ui + 3) && lc < 2; x++) {
                    uint8_t o = board_offset(x, y);
                    if((board->values[o] == 0) && (board->options[o] & m)) {
                        lc += 1;
                        lx = x;
                        ly = y;
                    }
                }
            }

            /* If only one possible cell, set its value. */
            if(lc == 1) {
                return board_set(board, lx, ly, l) ? 1 : 0;
            }
        }
    }

    ////////////////////////////////////////////////////////////////////
    // Now do the same for horizontal lines
    for(y = 0; y < 9; y++) {
        // Checks if the hline is already full
        if(board->hline_count[y] == 9) {
            continue;
        }

        for (l = 1; l < 10; l++) { // Iterate on possible values
            uint8_t lx = 0, ly = 0, lc = 0;
            uint16_t m = value_to_option(l);

            // Skip if the value is already used in the hline
            if ((m & board->hline_options[y]) == 0) {
                continue;
            }

            // Find if there's only one possible cell
            for(x = 0; x < 9 && lc < 2; x++) {
                uint8_t o = board_offset(x, y);
                if((board->values[o] == 0) && (board->options[o] & m)) {
                    lc += 1;
                    lx = x;
                    ly = y;
                }
            }

            if(lc == 1) { // If only one possible cell, set its value.
                return board_set(board, lx, ly, l) ? 1 : 0;
            }
        }
    }

    ////////////////////////////////////////////////////////////////////
    // Now do the same for vertical lines
    for(x = 0; x < 9; x++) {
        // Checks if the vline is already full
        if(board->vline_count[x] == 9) {
            continue;
        }

        for (l = 1; l < 10; l++) { // Iterate on possible values
            uint8_t lx = 0, ly = 0, lc = 0;
            uint16_t m = value_to_option(l);

            // Skip if the value is already used in the vline
            if ((m & board->vline_options[x]) == 0) {
                continue;
            }

            // Find if there's only one possible cell
            for(y = 0; y < 9 && lc < 2; y++) {
                uint8_t o = board_offset(x, y);
                if((board->values[o] == 0) && (board->options[o] & m)) {
                    lc += 1;
                    lx = x;
                    ly = y;
                }
            }

            if(lc == 1) { // If only one possible cell, set its value.
                return board_set(board, lx, ly, l) ? 1 : 0;
            }
        }
    }

    return 0;
}

int board_check_single(board_t *board)
{
    while(board->modified) {
        board->modified = 0;
        if(board_check_single_step(board)) {
            return 1;
        }
    }

    return 0;
}

int propagate(board_t *board, uint8_t i, uint8_t j, uint8_t v, uint16_t opt)
{
    uint8_t x, y;
    // (ui, uj) is the unit's upper-left coordinate
    uint8_t ui = (i / 3) * 3, uj = (j / 3) * 3;

    // Propagate change in the unit
    for (y = uj; y < (uj + 3); y++) {
        for (x = ui; x < (ui + 3); x++) {
            if(!(x == i && y == j)) {
                if(test_cell(board, x, y, v, opt)) {
                    return 1;
                }
            }
        }
    }

    // Propagate change in the horizontal line
    for (x = 0; x < 9; x++) {
        if(x != i) {
            if(test_cell(board, x, j, v, opt)) {
                return 1;
            }
        }
    }

    // Propagate change in the vertical line
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
    uint8_t u = (i / 3) + 3 * (j / 3);
    uint16_t opt = value_to_option(v);

    board->values[offset] = v;
    board->options[offset] = 0;
    board->options_count[offset] = 0;

    board->unit_count[u]++;
    board->unit_options[u] &= (o_mask ^ opt);

    board->hline_count[j]++;
    board->hline_options[j] &= (o_mask ^ opt);

    board->vline_count[i]++;
    board->vline_options[i] &= (o_mask ^ opt);

    board->modified = 1;
    board->count--;

    /* Propagate cell change to unit, horizontal and vertical line. */
    if(propagate(board, i, j, v, opt)) {
        return 1;
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
        uint8_t u, l, offset = 0, lx = 0, ly = 0, u_max = 0, o_min = 9;
        board_t current;

        // Identify the maximum number of indentified squares per
        // unit (except those completely identified).
        for (u = 0; u < 9; u++) {
            if(board->unit_count[u] > u_max && board->unit_count[u] < 9) {
                u_max = board->unit_count[u];
            }
        }

        // Find the square with the minimum number of possible options
        // within a unit with the maximum number of identified
        // squares.
        for (u = 0; u < 9; u++) {
            if(board->unit_count[u] != u_max) {
                continue;
            }

            uint8_t x, y, ui = (u % 3) * 3, uj = (u / 3) * 3;
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

        // Use this square to explore recursively
        for (l = 1; l < 10; l++) {
            uint16_t m = value_to_option(l);
            // We iterate on the valid options for the square
            if(!(m & board->options[offset])) {
                continue;
            }

            board_copy(&current, board); // Work on a copy of the board
            if(board_set(&current, lx, ly, l) == 0 &&
               board_check_single(&current) == 0 &&
               board_solve(&current) == 0) {
                // The value we tried was a success copy back the result
                board_copy(board, &current);
                return 0;
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
            if(board_solve(&board) != 0) {
                fprintf(stderr, "Error solving\n");
            }
            if(verbose) {
                board_dump(stdout, &board);
            }
        }
    }

    fclose(f);
    free(line);
    exit(0);
}
