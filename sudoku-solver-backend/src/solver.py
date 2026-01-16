from typing import List

CELL_MAP = [
    0,0,0, 1,1,1, 2,2,2,
    0,0,0, 1,1,1, 2,2,2,
    0,0,0, 1,1,1, 2,2,2,

    3,3,3, 4,4,4, 5,5,5,
    3,3,3, 4,4,4, 5,5,5,
    3,3,3, 4,4,4, 5,5,5,

    6,6,6, 7,7,7, 8,8,8,
    6,6,6, 7,7,7, 8,8,8,
    6,6,6, 7,7,7, 8,8,8,
]

# ---
def done(p: list) -> bool:
    for x in p:
        if x == 0:
            return False
        
    return True

def print_paper(p: list, w: int = 1):
    for i in range(81):
        if p[i] == 0:
            s = '_'
        else:
            s = str(p[i])

        if w == 1:
            print(s, end=' ')
        else:
            print(f'{s:.^{w}}', end=' ')
    
        if (i + 1) % 3 == 0:
            print('  ', end=' ')
        if (i + 1) % 9 == 0:
            print()
        if (i + 1) % 27 == 0:
            print()

# ---
def get_cell(i: int) -> int:        
    return CELL_MAP[i]

def cell_contains(p: list, cell: int, n: int) -> bool:
    c_row = cell // 3
    c_col = cell % 3
    
    off = c_row * 27 + c_col * 3

    for _ in range(3):
        for _ in range(3):
            if p[off] == n:
                return True
            off += 1
        off += 6

    return False

def cell_contains2(p: list, cell: int, n: int) -> bool:
    c_row = cell // 3
    c_col = cell % 3
    
    start = c_row * 27 + c_col * 3

    for x in range(start, start + 27, 9):
        for off in range(x, x + 3):
            if p[off] == n:
                return True

    return False

def row_contains(p: list, row: int, n: int) -> bool:
    start = row * 9

    for off in range(start, start + 9):
        if p[off] == n:
            return True
        
    return False

def col_contains(p: list, col: int, n: int) -> bool:
    for off in range(col, col + 81, 9):
        if p[off] == n:
            return True

    return False

# ---
def remove_possibility(possibilities: List[List[int]], i: int, n: int):
    possibilities[i].clear()

    cell = get_cell(i)
    row = i // 9
    col = i % 9

    # cell
    c_row = cell // 3
    c_col = cell % 3
    
    start = c_row * 27 + c_col * 3

    for x in range(start, start + 27, 9):
        for off in range(x, x + 3):
            if n in possibilities[off]:
                possibilities[off].remove(n)
            off += 1
        off += 6

    # row
    start = row * 9
    for off in range(start, start + 9):
        if n in possibilities[off]:
            possibilities[off].remove(n)
        off += 1

    # col
    for off in range(col, col + 81, 9):
        if n in possibilities[off]:
            possibilities[off].remove(n)
        off += 9

# ---
def solve(p: list) -> bool:
    possibilities: List[List[int]] = [[] for i in range(81)]

    MAX_COUNT = 82
    c = 0

    possibilities = [[] for _ in range(81)]

    for i, digit in enumerate(p):
        if digit != 0:
            continue

        cell = get_cell(i)
        row = i // 9
        col = i % 9

        for n in range(10):
            if cell_contains2(p, cell, n):
                continue
            if row_contains(p, row, n):
                continue
            if col_contains(p, col, n):
                continue

            possibilities[i].append(n) 
    
    while not done(p) and c < MAX_COUNT:
        updated = False

        for i, cell_possibilities in enumerate(possibilities):
            if len(cell_possibilities) == 1:
                p[i] = cell_possibilities[0]

                remove_possibility(possibilities, i, cell_possibilities[0])
                updated = True

        for x in range(3):              # cell row
            for y in range(3):          # cell col
                N = [[] for _ in range(10)]

                off = x * 27 + y * 3
                for _ in range(3):
                    for _ in range(3):
                        for possibility in possibilities[off]:
                            N[possibility - 1].append(off)
                        off += 1
                    off += 6

                for i, possiblities_of_n in enumerate(N):
                    if len(possiblities_of_n) == 1:
                        p[possiblities_of_n[0]] = i + 1

                        remove_possibility(possibilities, possiblities_of_n[0], i + 1)
                        updated = True

        for x in range(9):
            N = [[] for _ in range(10)]

            off = x * 9
            for y in range(9):
                for possibility in possibilities[off]:
                    N[possibility - 1].append(off)
                off += 1 

            
            for i, possiblities_of_n in enumerate(N):
                if len(possiblities_of_n) == 1:
                    p[possiblities_of_n[0]] = i + 1

                    remove_possibility(possibilities, possiblities_of_n[0], i + 1)
                    updated = True

        for x in range(9):
            N = [[] for _ in range(10)]

            off = x
            for y in range(9):
                for possibility in possibilities[off]:
                    N[possibility - 1].append(off)
                off += 9

            
            for i, possiblities_of_n in enumerate(N):
                if len(possiblities_of_n) == 1:
                    p[possiblities_of_n[0]] = i + 1

                    
                    remove_possibility(possibilities, possiblities_of_n[0], i + 1)
                    updated = True

        if not updated:
            break

        c += 1

    return done(p)
