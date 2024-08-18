#pragma once

#include <array>
#include <condition_variable>
#include <cstring>
#include <iostream>
#include <list>
#include <mutex>
#include <span>
#include <stdexcept>
#include <stop_token>
#include <thread>
#include <vector>

constexpr auto batchSize = 1000'000;
constexpr auto restSize = 10'000;

using BatchArray = std::array<char, batchSize>;

struct Split {
    Split(const Split &) = delete;
    Split(Split &&) = default;
    Split &operator=(const Split &) = delete;
    Split &operator=(Split &&) = default;
    ~Split() = default;

    Split() {}

    std::string snipRest() {
        auto f = data.rfind('\n');
        if (f == std::string::npos) {
            return {};
        }
        auto str = data.substr(f + 1);
        data.resize(data.size() + str.size());
        return str;
    }

    std::string data;
};

struct Allocator {
    size_t normalSize = 10;
    std::list<Split> allocationQueue{};
    std::jthread thread;
    std::mutex mutex;
    std::condition_variable cv;
    bool stopped = false;

    Allocator()
        : thread{[this](auto stop) { start(stop); }} {}

    ~Allocator() {
        stopped = true;
        cv.notify_one();

        // std::cout << "stopping with " << allocationQueue.size()
        //           << " splits in queue\n";
    }

    void start(std::stop_token token) {
        while (!token.stop_requested()) {
            auto lock = std::unique_lock{mutex};
            allocationQueue.emplace_back();
            allocationQueue.back().data.resize(batchSize + restSize);
            cv.wait(lock, [this]() {
                return allocationQueue.size() < normalSize || stopped;
            });
        }
    }

    Split get() {
        auto s = [this] {
            auto lock = std::unique_lock{mutex};
            auto s = std::move(allocationQueue.front());
            allocationQueue.pop_front();

            return s;
        }();
        cv.notify_one(); // Create more if needed
        return s;
    }
};

// Processes data that has been read from the file
struct BatchSplitter {
    Allocator allocator;

    std::mutex mutex;
    std::array<char, 1000> rest{};
    size_t restSize = 0;
    std::jthread processingThread;
    std::condition_variable cv;
    bool newDataToProcess = false;
    bool stopped = false;
    std::span<char> rawData;

    BatchSplitter()
        : processingThread{[this](auto stop) { process(stop); }} {}

    ~BatchSplitter() {
        stopped = true;
        cv.notify_one();
    }

    void process(std::stop_token token) {
        while (!token.stop_requested()) {
            auto lock = std::unique_lock{mutex};
            cv.wait(lock, [this]() { return stopped || newDataToProcess; });
            // std::cout << "process\n";

            auto s = allocator.get(); // The allocator might just be to
            // complicated
            // auto s = Split{};

            std::memcpy(s.data.data(), rest.data(), restSize);
            std::memcpy(
                s.data.data() + restSize, rawData.data(), rawData.size());

            // for (size_t i = 0; i < restSize; ++i) {
            //     s.data.push_back(rest[i]);
            // }
            // for (auto d : rawData) {
            //     s.data.push_back(d);
            // }

            // for (size_t i = 0; i < rawData.size(); ++i) {
            //     s.data[i + restSize] = rawData[i];
            // }
            restSize = 0;

            newDataToProcess = false;
            rawData = {};
        }
    }

    // This assumes that reading from disk is the slowest part in the process
    void push(std::span<char> data) {
        if (!rawData.empty()) {
            throw std::runtime_error{"race condition.."};
        }
        rawData = data;
        newDataToProcess = true;
        cv.notify_one();
    }
};
