#include "batchsplitter.h"
#include "processor.h"
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

struct Settings {
    std::filesystem::path path;

    Settings(int argc, char *argv[]) {
        auto args = std::vector<std::string>{argv + 1, argv + argc};

        for (size_t i = 0; i < args.size(); ++i) {
            auto arg = args.at(i);

            if (arg == "--help" || arg == "-h") {
                std::cout << "help string here\n";
                std::exit(0);
            }
            else {
                path = arg;
            }
        }
    }
};

int main(int argc, char *argv[]) {
    auto arg = Settings{argc, argv};

    std::cout << arg.path << std::endl;

    auto start = std::chrono::high_resolution_clock::now();

    // auto splitter = BatchSplitter{};
    // auto thread = std::thread{[]() {}};
    // thread.join();

    auto file = std::ifstream{arg.path};

    if (!file) {
        std::cerr << "could not open file\n";
        std::terminate();
    }

    auto batches = 0;

    // for (std::string line; std::getline(file, line);) {
    // }

    // constexpr auto batchSize = 1000'000;
    // for (auto batch = std::array<char, batchSize>{};
    //      file.read(batch.data(), batchSize);) {
    //     ++batches;
    // }

    // auto i = 0;
    // constexpr auto batchSize = 1000'000;
    // constexpr int numBatches = 4;
    // auto batch = std::array<std::array<char, batchSize>, numBatches>{};
    // for (; file.read(batch.at(i).data(), batchSize);) {
    //     // std::cout << "read\n";
    //     ++batches;
    //     splitter.push(batch.at(i));
    //     ++i;
    //     i = i % numBatches;
    //     // auto lock = std::unique_lock{m};
    // }

    // constexpr auto batchSize = 1000'000;
    // auto batch = std::make_unique<std::array<char, batchSize>>();
    // for (; file.read(batch->data(), batchSize);) {
    //     ++batches;
    // }

    // constexpr auto batchSize = 1000'000;
    // auto batch = new char[batchSize];
    // for (; file.read(batch, batchSize);) {
    //     ++batches;
    // }

    // constexpr auto batchSize = 1000'000;
    // auto batch = std::vector<char>{};
    // batch.resize(batchSize);
    // for (; file.read(batch.data(), batchSize);) {
    // }

    auto processor = Processor{};
    // auto allocator = Allocator{};

    // for (auto s = Split{};
    //      s = allocator.get(), file.read(s.data.data(), 1000'000);) {
    //     processor.process(std::move(s));
    //     ++batches;
    // }

    // constexpr auto batchSize = 1000;

    std::string rest;

    for (; file;) {
        auto s = Split{};

        if (rest.empty()) {
            s.data.resize(batchSize);
            file.read(s.data.data(), batchSize);
        }
        else {
            s.data.resize(batchSize + rest.size() + 1);
            for (size_t i = 0; i < rest.size(); ++i) {
                s.data.at(i) = rest.at(i);
            }
            s.data.at(rest.size()) = '\n';
            file.read(s.data.data() + rest.size() + 1, batchSize);
        }

        file.read(s.data.data(), batchSize);
        auto rest = s.snipRest();
        processor.process(std::move(s));
        ++batches;
    }

    auto stop = std::chrono::high_resolution_clock::now();

    std::cout << "batches " << batches << "\n";

    {
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);

        std::cout << "duration: " << duration.count() << "ms\n";
    }
    {
        auto duration =
            std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

        std::cout << "duration: " << duration.count() << "us\n";
    }

    return 0;
}
