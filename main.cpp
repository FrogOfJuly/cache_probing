//
// Created by Kirill Golubev on 11.03.2023.
//

#include <iostream>
#include <chrono>
#include <vector>
#include <numeric>
#include <set>
#include <tuple>
#include <algorithm>
#include <optional>

constexpr int million = 1000 * 1000;

constexpr int Z = 1024 * 1024 * 1024; // Max memory
constexpr int N = 16; // Max associativity

namespace timer {
    char *data = (char *) malloc(Z);

    long long measure(int H, int S) {
        char **x;
        for (int i = (S - 1) * H; i >= 0; i -= H) {
            char *next;
            x = (char **) &data[i];
            if (i >= H) {
                next = &data[i - H];
            } else {
                next = &data[(S - 1) * H];
            }
            *x = next;
        }

        const long long sample_num = 20;
        std::vector<long long> ans;
        for (long long k = 0L; k < sample_num; k++) {
            auto start = std::chrono::high_resolution_clock::now();
            for (long long i = 0L; i < million; i++) {
                x = (char **) *x;
            }
            auto end = std::chrono::high_resolution_clock::now();
            ans.push_back((end - start).count());
        }
        return std::accumulate(ans.begin(), ans.end(), 0L) / sample_num;
    }
}

bool deltaDiff(long long cur, long long prev){
    return (cur - prev)*10 > cur;
}

std::pair<std::vector<std::set<int>>, int> record_jumps(int H_ini, int S_ini){
    std::vector<std::set<int>> jumps;
    int H = H_ini;

    for (; H < Z / N; H *= 2) {
        std::cout << "probing stride " << H << "..." << std::endl;
        auto prev_time = timer::measure(H, S_ini);
        std::set<int> new_jumps;

        for (int S = S_ini; S <= N; S++) {
            auto curr_time = timer::measure(H, S);
            std::cout << curr_time << " ";
            if (deltaDiff(curr_time, prev_time)) {
                new_jumps.insert(S - 1);
                std::cout << S - 1 << " ";
            }
            prev_time = curr_time;
        }
        std::cout << std::endl;

        bool same = true;
        if (not jumps.empty()) {
            for (int jump: new_jumps) {
                same &= jumps.back().count(jump);
            }
            for (int jump: jumps.back()) {
                same &= new_jumps.count(jump);
            }
        }

        if (same && H >= 256 * 1024) {
            break;
        }

        jumps.push_back(new_jumps);
    }

    return {jumps, H};
}
std::vector<std::pair<int, int>> build_caches(std::vector<std::set<int>> jumps, int H){
    std::vector<std::pair<int, int>> caches;

    std::set<int> to_process = jumps[jumps.size() - 1];
    std::reverse(jumps.begin(), jumps.end());
    for (auto &jump: jumps) {
        std::set<int> to_delete;
        for (int s: to_process) {
            if (!jump.count(s)) {
                caches.push_back({H * s, s});
                to_delete.insert(s);
            }
        }
        for (int s: to_delete) {
            to_process.erase(s);
        }
        H /= 2;
    }

    std::sort(caches.begin(), caches.end());
    return caches;
}

int get_line_size(int cache_size, int cache_assoc){
    int cache_line_size = -1;
    int prev_first_jump = 1025;

    for (int L = 1; L <= cache_size; L *= 2) {
        auto prev_time = timer::measure(cache_size / cache_assoc + L, 2);
        int first_jump = -1;
        std::cout << "probing stride " << cache_size / cache_assoc + L << "..." << std::endl;
        for (int S = 1; S <= 1024; S *= 2) {
            auto curr_time = timer::measure(cache_size / cache_assoc + L, S + 1);
            if (deltaDiff(curr_time, prev_time)) {
                if (first_jump <= 0) {
                    first_jump = S;
                }
                std::cout << S << " ";
            }
            prev_time = curr_time;
        }
        std::cout << std::endl;
        if (first_jump > prev_first_jump) {
            cache_line_size = L * cache_assoc;
            break;
        }
        prev_first_jump = first_jump;
    }

    return cache_line_size;
}

int main() {
    constexpr int H_ini = 16;
    constexpr int S_ini = 1;

    auto [jumps, H] = record_jumps(H_ini, S_ini);
    auto caches = build_caches(std::move(jumps), H);

    if (caches.empty()){
        throw std::runtime_error("No caches:(");
    }

    for (auto i = 0; i < caches.size(); i++) {
        int cache_size = caches[i].first;
        int cache_assoc = caches[i].second;
        int cache_line_size = get_line_size(cache_size, cache_assoc);

        std::cout << "L" << i + 1 << " cache: " << "size = " << cache_size <<
                  ", associativity = " << cache_assoc <<
                  ", line size = " << cache_line_size << std::endl;
    }
}