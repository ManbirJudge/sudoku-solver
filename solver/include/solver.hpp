#ifndef SOLVER_HPP
#define SOLVER_HPP

#include <array>

using Paper = std::array<int, 81>;

void print_paper(const Paper &p);

bool is_solved(const Paper &p);

bool solve(Paper &p);

#endif