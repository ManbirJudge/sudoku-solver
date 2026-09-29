#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>

#include "solver.hpp"

int main(void) {
    size_t N_PAPERS = 80000;

    std::vector<Paper> papers;
    std::vector<Paper> solutions;
    papers.reserve(N_PAPERS);

    size_t n_invalid_solves = 0;
    size_t n_unsolved = 0;

    // loading
    std::cout << "Loading...\n";

    std::ifstream f("db/games.csv");
    if (!f.is_open()) {
        std::cerr << "Error: Failed to open 'db/games.csv'.\n";
        return 1;
    }

    std::string line;
    std::getline(f, line);

    int c = 0;
    Paper p;
    char chr;
    while (std::getline(f, line) && c < N_PAPERS) {
        std::stringstream ss(line);
        std::string col;
        std::getline(ss, col, ',');

        std::getline(ss, col, ',');
        for (int j = 0; j < 81; j++) {
            chr = col[j];
            p[j] = (chr == '.') ? 0 : (chr - '0');
        }
        papers.push_back(p);
        
        std::getline(ss, col, ',');
        for (int j = 0; j < 81; j++) {
            chr = col[j];
            p[j] = (chr == '.') ? 0 : (chr - '0');
        }
        solutions.push_back(p);

        c++;
    }

    // solving
    std::cout << "Staring...\n";

    const auto start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < papers.size(); i++) {
        if (i % 10 == 0) std::cout << '\r' << i << std::flush;
        if (solve(papers[i])) {
            if (papers[i] != solutions[i]) n_invalid_solves++;
        }
        else n_unsolved++;
    }
    std::cout << "\r                   \r";

    // metrics
    const auto end = std::chrono::high_resolution_clock::now();
    
    const std::chrono::duration<double> dur = end - start;
    const size_t n_solved = papers.size() - n_unsolved - n_invalid_solves;
    const double solved_pct = (double)n_solved / papers.size() * 100.;
    const double unsolved_pct = (double)n_unsolved / papers.size() * 100.;
    const double invalid_solves_pct = (double)n_invalid_solves / papers.size() * 100.;
    const double rate = papers.size() / dur.count();

    std::cout << "Done\n-----------------\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "time taken = " << dur.count() << " s\n";
    std::cout << "processed  = " << papers.size() << '\n';
    std::cout << "solved     = " << n_solved << " (" << solved_pct << "%)\n";
    std::cout << "not solved = " << n_unsolved << " (" << unsolved_pct << "%)\n";
    std::cout << "inval sols = " << n_invalid_solves << " (" << invalid_solves_pct << "%)\n";
    std::cout << "rate       = " << rate << "/s\n";

    // ---
    return 0;
}