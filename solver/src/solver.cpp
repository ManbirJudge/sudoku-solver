#include <algorithm>
#include <array>
#include <iostream>
#include <set>

#include "solver.hpp"

const std::array<int, 81> BLOCK_MAP = {
    0,0,0, 1,1,1, 2,2,2,
    0,0,0, 1,1,1, 2,2,2,
    0,0,0, 1,1,1, 2,2,2,

    3,3,3, 4,4,4, 5,5,5,
    3,3,3, 4,4,4, 5,5,5,
    3,3,3, 4,4,4, 5,5,5,

    6,6,6, 7,7,7, 8,8,8,
    6,6,6, 7,7,7, 8,8,8,
    6,6,6, 7,7,7, 8,8,8
};

// ---
void print_paper(const Paper &p) {
    char _;
    for (int i = 0; i < 81; i++) {
        if (p[i] == 0) _ = '_';
        else _ = p[i] + '0';

        std::cout << _;
        
        if ((i + 1) % 3 == 0)  std::cout << ' ';
        if ((i + 1) % 9 == 0)  std::cout << '\n';
        if ((i + 1) % 27 == 0) std::cout << '\n';
    }
}

// ---
bool is_solved(const Paper &p) {
    return !std::any_of(p.begin(), p.end(), [](int x) {
        return x == 0;
    });
}

int get_block(int cell) {
    return BLOCK_MAP[cell];
}

// ---
int block_contains(const Paper &p, int block, int n) {
    int b_row = block / 3;
    int b_col = block % 3;

    int start = b_row * 27 + b_col * 3;
    for (int x = start; x < start + 27; x += 9) {
        for (int cell = x; cell < x + 3; cell++) {
            if (p[cell] == n) return cell;
        }
    }

    return -1;
}

int row_contains(const Paper &p, int row, int n) {
    int start = row * 9;
    for (int cell = start; cell < start + 9; cell++)
        if (p[cell] == n) return cell;

    return -1;
}

int col_contains(const Paper &p, int col, int n) {
    for (int cell = col; cell < col + 81; cell += 9)
        if (p[cell] == n) return cell;

    return -1;
}

// ---
void remove_possibilities(std::array<std::set<int>, 81> &possibilities, int cell, int n) {
    possibilities[cell].clear();
    
    int block = get_block(cell);
    int row   = cell / 9;
    int col   = cell % 9;

    // block
    int b_row = block / 3;
    int b_col = block % 3;

    int start = b_row * 27 + b_col * 3;
    for (int x = start; x < start + 27; x += 9) {
        for (int cell_ = x; cell_ < x + 3; cell_++)
            possibilities[cell_].erase(n);
    }

    // row
    start = row * 9;
    for (int cell_ = start; cell_ < start + 9; cell_++)
        possibilities[cell_].erase(n);

    // col
    for (int cell_ = col; cell_ < col + 81; cell_ += 9)
        possibilities[cell_].erase(n);
}

// ---
bool solve(Paper &p) {
    int n_iter = 0;
    
    // initializing possibilities
    std::array<std::set<int>, 81> possibilities{};

    for (int cell = 0; cell < 81; cell++) {
        int n = p[cell];

        if (n != 0) continue;  // obviously, skip filled cells 

        int block = get_block(cell);
        int row   = cell / 9;
        int col   = cell % 9;

        for (int possible_n = 1; possible_n <= 9; possible_n++) {
            if (
                block_contains(p, block, possible_n) != -1 ||
                row_contains(p, row, possible_n) != -1 ||
                col_contains(p, col, possible_n) != -1
            ) continue;
            possibilities[cell].insert(possible_n);
        }
    }

    // solver loop
    while (!is_solved(p) && n_iter < 82) {
        bool updated = false;

        // naked single
        for (int cell = 0; cell < 81; cell++) {
            if (possibilities[cell].size() == 1) {
                p[cell] = *possibilities[cell].begin();

                remove_possibilities(possibilities, cell, p[cell]);
                updated = true;
            }
        }

        // hidden single
        std::array<std::set<int>, 9> possible_cells{};

        const auto fill_hidden_singles = [&p, &possibilities, &possible_cells, &updated]() {
            for (int i = 1; i <= 9; i++) {
                if (possible_cells[i - 1].size() == 1) {
                    int cell_ = *possible_cells[i - 1].begin();

                    p[cell_] = i;

                    remove_possibilities(possibilities, cell_, i);
                    updated = true;

                    // return;  // stale state - potential BUG? IDK? 
                }
            }
        };

        for (int b_row = 0; b_row < 3; b_row++) {
            for (int b_col = 0; b_col < 3; b_col++) {
                possible_cells.fill({});

                int start = b_row * 27 + b_col * 3;
                for (int x = start; x < start + 27; x += 9) {
                    for (int cell = x; cell < x + 3; cell++) {
                        for (int possible_n : possibilities[cell])
                            possible_cells[possible_n - 1].insert(cell);
                    }
                }

                fill_hidden_singles();
            }
        }

        for (int u = 0; u < 9; u++) {
            possible_cells.fill({});

            int start = u * 9;
            for (int cell = start; cell < start + 9; cell++) {
                for (int possible_n : possibilities[cell])
                    possible_cells[possible_n - 1].insert(cell);
            }

            fill_hidden_singles();
        }
        
        for (int u = 0; u < 9; u++) {
            possible_cells.fill({});

            for (int cell = u; cell < u + 81; cell += 9) {
                for (int possible_n : possibilities[cell])
                    possible_cells[possible_n - 1].insert(cell);
            }

            fill_hidden_singles();
        }

        // ---
        n_iter++;
        if (!updated) break;
    }

    // back track and result
    if (!is_solved(p)) {
        int best_cell = -1;
        size_t min_options = 6967;

        for (int cell = 0; cell < 81; cell++) {
            if (p[cell] != 0) continue;
            if (possibilities[cell].empty()) return false;  // dead end

            if (possibilities[cell].size() < min_options) {
                min_options = possibilities[cell].size();
                best_cell = cell;
            }
        }

        if (best_cell == -1) return true;  // no empty cell - this is a theoretically impossible state for the program bcz of previous check but whatever

        const Paper prev_p = p;
        for (int n : possibilities[best_cell]) {
            p[best_cell] = n;

            if (solve(p)) return true;

            p = prev_p;  // multiple cells might be changed
        }

        return false; // all guesses lead to dead-end
    }

    return true;
}