#pragma once

#include <array>
#include <mutex>
#include <span>

constexpr auto batchSize = 1000'000;

using BatchArray = std::array<char, batchSize>;

inline auto rest = std::array<char, 1000>{};

struct BatchSplitter {
    std::mutex mutex;

    void push(std::span<char> data) {
        auto lock = std::unique_lock{mutex};
    }
};
