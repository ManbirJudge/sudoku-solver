import csv
import time

from solver import solve, print_paper

# ---
# if __name__ == '__main__':
    # N_PAPERS = 1_000_000
    # papers: List[List[int]] = []
    # soln_not_found: List[int] = []

    # # reading
    # with open('games.csv', newline='') as f:
    #     reader = csv.reader(f)
    #     next(reader)

    #     for i, row in enumerate(reader):
    #         if i >= N_PAPERS:
    #             break
    #         papers.append([int(c) for c in str(row[0])])

    # # ---
    # print('Starting.')
    # start = time.perf_counter()

    # for i in range(N_PAPERS):
    #     if not solve(papers[i]):
    #         soln_not_found.append(i)

    # # metrics and result
    # end = time.perf_counter()
    # duration = end - start
    # rate = N_PAPERS / duration

    # print('Done.\n---------------')
    # print(f'Time taken: {duration:.4} s')
    # print(f'Processed: {N_PAPERS}')
    # print(f'Solved: {N_PAPERS - len(soln_not_found)} {((N_PAPERS - len(soln_not_found)) / N_PAPERS * 100):.2f}%')
    # print(f'Not solved: {len(soln_not_found)} {(len(soln_not_found) / N_PAPERS * 100):.2f}%')
    # print(f'Rate: {rate:.4}/s')
    # print(f'Not solved lsit: {soln_not_found}')

if __name__ == '__main__':
    paper = [
        0,0,7, 0,0,0, 0,0,8,
        0,9,3, 4,0,0, 0,0,0,
        0,0,0, 0,0,2, 0,4,0,

        0,0,0, 0,0,8, 2,7,1,
        0,7,0, 0,0,0, 0,9,0,
        1,3,6, 2,0,0, 0,0,0,

        0,2,0, 5,0,0, 0,0,0,
        0,0,0, 0,0,7, 4,5,0,
        3,0,0, 0,0,0, 8,0,0,
    ]


    print('Starting.')
    print_paper(paper)

    start = time.perf_counter()
    solve(paper)
    end = time.perf_counter()

    print(f'Done in {(end - start):.4f} seconds')
    print_paper(paper)