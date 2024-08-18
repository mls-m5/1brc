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
    auto settings = Settings{argc, argv};

    std::cout << settings.path << std::endl;

    auto start = std::chrono::high_resolution_clock::now();

    // auto splitter = BatchSplitter{};
    // auto thread = std::thread{[]() {}};
    // thread.join();

    auto file = std::ifstream{settings.path};

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

    // {
    //     std::string rest;

    //     for (; file;) {
    //         auto s = Split{};

    //         if (rest.empty()) {
    //             s.data.resize(batchSize);
    //             file.read(s.data.data(), batchSize);
    //         }
    //         else {
    //             s.data.resize(batchSize + rest.size() + 1);
    //             for (size_t i = 0; i < rest.size(); ++i) {
    //                 s.data.at(i) = rest.at(i);
    //             }
    //             s.data.at(rest.size()) = '\n';
    //             file.read(s.data.data() + rest.size() + 1, batchSize);
    //         }

    //         file.read(s.data.data(), batchSize);
    //         auto rest = s.snipRest();
    //         processor.process(std::move(s));
    //         ++batches;
    //     }
    // }

    size_t fileLength = 0;

    auto stops = std::vector<size_t>{};

    {
        {

            // stops.push_back(0);
            // ate means to go for the end of the file
            std::ifstream file(settings.path, std::ios::binary | std::ios::ate);
            fileLength = file.tellg();

            std::cout << "file length " << fileLength << "\n";

            stops.resize(fileLength / batchSize);

            for (size_t i = 0; i < stops.size(); ++i) {
                stops.at(i) = i * fileLength / stops.size();
            }

            std::cout << "number of stops << " << stops.size() << "\n";

            auto data = std::array<char, 300>{};
            for (auto &s : stops) {
                file.seekg(s);
                file.read(data.data(), data.size());
                for (size_t i = 0; i < data.size(); ++i) {
                    if (data.at(i) == '\n') {
                        s += i;
                        break;
                    }
                }

                // std::cout << "before: '";
                // std::cout.write(data.data(), 50);
                // std::cout << "'\n";

                // file.seekg(s);
                // file.read(data.data(), data.size());
                // std::cout << "after: '";
                // std::cout.write(data.data(), 50);
                // std::cout << "'\n";
            }

            stops.push_back(fileLength);
        }
    }

    std::cout << "stops processed" << std::endl;

    auto ranges = std::vector<std::pair<size_t, size_t>>{};
    {
        size_t lastPos = 0;
        auto pushRange = [&](size_t pos) {
            ranges.push_back({lastPos, pos});
            lastPos = pos;
        };
        for (auto stop : stops) {
            pushRange(stop);
        }
    }

    auto threadRanges = std::vector<std::vector<std::pair<size_t, size_t>>>{};

    auto numThreads = std::thread::hardware_concurrency();
    threadRanges.resize(numThreads);

    for (size_t i = 0, t = 0; i < ranges.size(); ++i, ++t, t = t % numThreads) {
        threadRanges.at(t).push_back(ranges.at(i));
    }

    {
        auto threads = std::list<std::jthread>{};

        for (auto &ranges : threadRanges) {
            threads.push_back(std::jthread{[ranges = std::move(ranges),
                                            &settings]() {
                auto processor = Processor{};
                std::cout << "thread " << std::this_thread::get_id() << "\n";
                // for (size_t i = 0; i < 10; ++i) {
                //     std::cout << range.at(i).first << ", " <<
                //     range.at(i).second
                //               << "\n";
                // }

                auto file = std::ifstream{settings.path, std::ios::binary};

                for (auto range : ranges) {
                    file.seekg(range.first);
                    auto s = Split{};
                    s.data.resize(range.second - range.first);

                    file.read(s.data.data(), s.data.size());
                    processor.process(std::move(s));
                }
            }});
        }
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
