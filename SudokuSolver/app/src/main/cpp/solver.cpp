#include "solver.h"

// ---
string paperToStr(int p[81], bool beautify) {
    stringstream sBuilder;

    if (beautify)
    {
        for (int i = 0; i < 81; i++) {
            string c = "_";
            if (p[i] != 0)
                c = to_string(p[i]);

            sBuilder << c << ' ';

            if ((i + 1) % 3 == 0)
                sBuilder << ' ';
            if ((i + 1) % 9 == 0)
                sBuilder << endl;
            if ((i + 1) % 27 == 0)
                sBuilder << endl;
        }
    } else {
        for (int i = 0; i < 81; i++)
            sBuilder << p[i];
    }

    return sBuilder.str();
}

string possibilitiesToStr(const vector<vector<int>>& cells) {
    if (cells.size() != 81) {
        throw invalid_argument("Input must contain exactly 81 cells.");
    }

    vector<string> cellStrings(81);
    for (int i = 0; i < 81; i++) {
        stringstream ss;
        for (int num : cells[i]) ss << num;
        cellStrings[i] = ss.str();
    }

    size_t maxWidth = 0;
    for (const auto& s : cellStrings) {
        maxWidth = max(maxWidth, s.size());
    }

    stringstream out;
    for (int r = 0; r < 9; r++) {
        for (int c = 0; c < 9; c++) {
            out << setw((int)maxWidth) << cellStrings[r * 9 + c];
            if (c < 8) out << " ";
        }
        out << "\n";
    }

    return out.str();
}

void logVectorOfVectors(const vector<vector<int>>& data) {
    for (size_t i = 0; i < data.size(); ++i) {
        string line = to_string(i) + ": ";
        for (int val : data[i])
            line += to_string(val) + " ";

        LOGD("%s", line.c_str());
    }
}

// ---
bool isSudokuDone(const int p[81]) {
    for (int i = 0; i < 81; i++) {
        if (p[i] == 0)
            return false;
    }

    return true;
}

bool isSudokuValid(const int p[81])
{
    (void)(p);
    return true;
}

// ---
bool boxHas(const int p[81], int box, int n) {
    return any_of(
        begin(BOX_OFFSETS[box]),
        end(BOX_OFFSETS[box]),
        [&](int off) { return p[off] == n; }
    );
}

bool rowHas(const int p[81], int row, int n) {
    return any_of(
        begin(ROW_OFFSETS[row]),
        end(ROW_OFFSETS[row]),
        [&](int off) { return p[off] == n; }
    );
}

bool colHas(const int p[81], int col, int n) {
    return any_of(
        begin(COL_OFFSETS[col]),
        end(COL_OFFSETS[col]),
        [&](int off) { return p[off] == n; }
    );
}

// ---
void removePossibility(vector<vector<int>> &possibilities, int i, int n, bool fromCell, bool fromRow, bool fromCol) {
    possibilities[i] = {};

    int box = BOX_MAP[i];
    int row = i / 9;
    int col = i % 9;

    // box
    if (fromCell) {
        for (const int off : BOX_OFFSETS[box]) {
            auto it = find(possibilities[off].begin(), possibilities[off].end(), n);
            if (it != possibilities[off].end())
                possibilities[off].erase(it);
        }
    }

    // row
    if (fromRow) {
        for (const int off : ROW_OFFSETS[row]) {
            auto it = find(possibilities[off].begin(), possibilities[off].end(), n);
            if (it != possibilities[off].end())
                possibilities[off].erase(it);
        }
    }

    // col
    if (fromCol) {
        for (const int off : COL_OFFSETS[col]) {
            auto it = find(possibilities[off].begin(), possibilities[off].end(), n);
            if (it != possibilities[off].end())
                possibilities[off].erase(it);
        }
    }
}

tuple<bool, int, int, vector<int>> find2VectorsWith2Common(const vector<vector<int>>& data)
{
    size_t n = data.size();

    for (int i = 0; i < n; i++) {
        unordered_set<int> setA(data[i].begin(), data[i].end());

        for (int j = i + 1; j < n; j++) {
            int commonCount = 0;
            vector<int> commonElements;

            for (int num : data[j]) {
                if (setA.count(num)) {
                    commonCount++;
                    commonElements.push_back(num);
                }
            }

            if (commonCount == 2) {
                return {true, i, j, commonElements};
            }
        }
    }

    return {false, -1, -1, {}};
}

tuple<bool, int, int, int> find2And3(const vector<vector<int>>& data)
{
    size_t n = data.size();

    for (int i = 0; i < n; i++) {
        if (data[i].size() != 2) continue;  // only size 2

        unordered_set<int> setA(data[i].begin(), data[i].end());

        for (int j = 0; j < n; j++) {
            if (i == j || data[j].size() != 3) continue; // only size 3

            unordered_set<int> setB(data[j].begin(), data[j].end());
            int commonCount = 0;
            int extra = -1;

            for (int num : setB) {
                if (setA.count(num)) {
                    commonCount++;
                } else {
                    extra = num;
                }
            }

            // Must match exactly 2 + 1 extra
            if (commonCount == 2) {
                // Verify that the common elements do NOT appear elsewhere
                bool valid = true;
                for (int k = 0; k < n; k++) {
                    if (k == i || k == j) continue;
                    for (int num : data[k]) {
                        if (setA.count(num)) {
                            valid = false; // common element found in another vector
                            break;
                        }
                    }
                    if (!valid) break;
                }

                if (valid) {
                    return {true, i, j, extra};
                }
            }
        }
    }

    return {false, -1, -1, -1};
}

// ---
bool solve(int p[81]) {
    vector<vector<int>> possibilities(81);
    size_t c = 0;

    vector<vector<int>> N(10);

    // calculating initial possibilities
    for (int i = 0; i < 81; i++) {
        if (p[i] != 0)
            continue;

        int box = BOX_MAP[i];
        int row = i / 9;
        int col = i % 9;

        for (int n = 1; n < 10; n++) {
            if (boxHas(p, box, n))
                continue;
            if (rowHas(p, row, n))
                continue;
            if (colHas(p, col, n))
                continue;

            possibilities[i].push_back(n);
        }
    }

    // solver loop
    while (c < MAX_COUNT) {
        c++;

        bool updated = false;

        // naked singles
        for (int i = 0; i < 81; i++) {
            if (possibilities[i].size() == 1) {
                p[i] = possibilities[i][0];

                removePossibility(possibilities, i, p[i]);
                updated = true;
            }
        }

        // hidden singles
        for (const auto& box : BOX_OFFSETS) {
            N.assign(10, {});

            for (const int off : box) {
                for (const int possibility: possibilities[off])
                    N[possibility - 1].push_back(off);
            }

            for (int i = 0; i < 10; i++) {
                if (N[i].size() == 1) {
                    p[N[i][0]] = i + 1;

                    removePossibility(possibilities, N[i][0], i + 1);
                    updated = true;
                }
            }
        }

        for (const auto& row : ROW_OFFSETS) {
            N.assign(10, {});

            for (const int off : row) {
                for (const int possibility: possibilities[off])
                    N[possibility - 1].push_back(off);
            }

            for (int i = 0; i < 10; i++) {
                if (N[i].size() == 1) {
                    p[N[i][0]] = i + 1;

                    removePossibility(possibilities, N[i][0], i + 1);
                    updated = true;
                }
            }
        }

        for (const auto& col : COL_OFFSETS) {
            N.assign(10, {});

            for (const int off : col) {
                for (const int possibility: possibilities[off])
                    N[possibility - 1].push_back(off);
            }

            for (int i = 0; i < 10; i++) {
                if (N[i].size() == 1) {
                    p[N[i][0]] = i + 1;

                    removePossibility(possibilities, N[i][0], i + 1);
                    updated = true;
                }
            }
        }

        if (updated) continue;

        // TODO: locked candidates

        // naked pair
        for (const auto& box : BOX_OFFSETS) {
            N.assign(10, {});

            for (const int off : box) {
                for (const int possibility: possibilities[off])
                    N[possibility - 1].push_back(off);
            }

            // ---
            auto z = find2And3(N);

            bool found = get<0>(z);
            // int α = get<1>(z);
            int β = get<2>(z);
            int γ = get<3>(z);

            if (found)
            {
//                LOGD("Possibilities:\n%s", possibilitiesToStr(possibilities).c_str());

                auto it = find(possibilities[γ].begin(), possibilities[γ].end(), β + 1);
                if (it != possibilities[γ].end())
                possibilities[γ].erase(it);

                updated = true;
            }
        }

        if (updated) continue;

        for (const auto& row : ROW_OFFSETS) {
            N.assign(10, {});

            for (const int off : row) {
                for (const int possibility: possibilities[off])
                    N[possibility - 1].push_back(off);
            }

            // ---
            auto z = find2And3(N);

            bool found = get<0>(z);
            // int α = get<1>(z);
            int β = get<2>(z);
            int γ = get<3>(z);

            if (found)
            {
//                LOGD("Possibilities:\n%s", possibilitiesToStr(possibilities).c_str());

                auto it = find(possibilities[γ].begin(), possibilities[γ].end(), β + 1);
                if (it != possibilities[γ].end())
                    possibilities[γ].erase(it);

                updated = true;
            }
        }

        if (updated) continue;

        for (const auto& col : COL_OFFSETS) {
                N.assign(10, {});

                for (const int off : col) {
                    for (const int possibility: possibilities[off])
                        N[possibility - 1].push_back(off);
                }

                // ---
                auto z = find2And3(N);

                bool found = get<0>(z);
                // int α = get<1>(z);
                int β = get<2>(z);
                int γ = get<3>(z);

                if (found)
                {
//                    LOGD("Possibilities:\n%s", possibilitiesToStr(possibilities).c_str());

                    auto it = find(possibilities[γ].begin(), possibilities[γ].end(), β + 1);
                    if (it != possibilities[γ].end())
                        possibilities[γ].erase(it);

                    updated = true;
                }
            }

        if (updated) continue;

        // TODO: hidden pair

        // ---
        break;
    }

    // ---
    LOGD("Number of steps to solve: %zu", c);

    return isSudokuDone(p);
}
