#ifndef SOLVER_H
#define SOLVER_H

#include <vector>
#include <unordered_set>
#include <tuple>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>

#include "base.h"

const int MAX_COUNT = 1000;
const int BOX_MAP[81] =
{
    0, 0, 0, 1, 1, 1, 2, 2, 2,
    0, 0, 0, 1, 1, 1, 2, 2, 2,
    0, 0, 0, 1, 1, 1, 2, 2, 2,

    3, 3, 3, 4, 4, 4, 5, 5, 5,
    3, 3, 3, 4, 4, 4, 5, 5, 5,
    3, 3, 3, 4, 4, 4, 5, 5, 5,

    6, 6, 6, 7, 7, 7, 8, 8, 8,
    6, 6, 6, 7, 7, 7, 8, 8, 8,
    6, 6, 6, 7, 7, 7, 8, 8, 8
};

const int BOX_OFFSETS[9][9] = {
    {0, 1, 2, 9, 10, 11, 18, 19, 20 },
    {3, 4, 5, 12, 13, 14, 21, 22, 23 },
    {6, 7, 8, 15, 16, 17, 24, 25, 26 },
    {27, 28, 29, 36, 37, 38, 45, 46, 47 },
    {30, 31, 32, 39, 40, 41, 48, 49, 50 },
    {33, 34, 35, 42, 43, 44, 51, 52, 53 },
    {54, 55, 56, 63, 64, 65, 72, 73, 74 },
    {57, 58, 59, 66, 67, 68, 75, 76, 77 },
    {60, 61, 62, 69, 70, 71, 78, 79, 80 }
};
const int ROW_OFFSETS[9][9] = {
    {0, 1, 2, 3, 4, 5, 6, 7, 8 },
    {9, 10, 11, 12, 13, 14, 15, 16, 17 },
    {18, 19, 20, 21, 22, 23, 24, 25, 26 },
    {27, 28, 29, 30, 31, 32, 33, 34, 35 },
    {36, 37, 38, 39, 40, 41, 42, 43, 44 },
    {45, 46, 47, 48, 49, 50, 51, 52, 53 },
    {54, 55, 56, 57, 58, 59, 60, 61, 62 },
    {63, 64, 65, 66, 67, 68, 69, 70, 71 },
    {72, 73, 74, 75, 76, 77, 78, 79, 80 }
};
const int COL_OFFSETS[9][9] = {
    {0, 9, 18, 27, 36, 45, 54, 63, 72 },
    {1, 10, 19, 28, 37, 46, 55, 64, 73 },
    {2, 11, 20, 29, 38, 47, 56, 65, 74 },
    {3, 12, 21, 30, 39, 48, 57, 66, 75 },
    {4, 13, 22, 31, 40, 49, 58, 67, 76 },
    {5, 14, 23, 32, 41, 50, 59, 68, 77 },
    {6, 15, 24, 33, 42, 51, 60, 69, 78 },
    {7, 16, 25, 34, 43, 52, 61, 70, 79 },
    {8, 17, 26, 35, 44, 53, 62, 71, 80 }
};

string paperToStr(int p[81], bool beautify = true);
void logVectorOfVectors(const vector<vector<int>>& data);

bool isSudokuDone(const int p[81]);
bool isSudokuValid(const int p[81]);

bool boxHas(const int p[81], int box, int n);
bool rowHas(const int p[81], int row, int n);
bool colHas(const int p[81], int col, int n);

void removePossibility(vector<vector<int>> &possibilities, int i, int n, bool fromCell = true, bool fromRow = true, bool fromCol = true);

tuple<bool, int, int, vector<int>> find2VectorsWith2Common(const vector<vector<int>>& data);

bool solve(int p[81]);

#endif