#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <list>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

// Got this by trial and error
constexpr auto batchSize = 1000'000;

struct Split {
    Split() = default;
    Split(const Split &) = delete;
    Split(Split &&) = default;
    Split &operator=(const Split &) = delete;
    Split &operator=(Split &&) = default;
    ~Split() = default;

    std::string data;
};

// Faster number parser
inline float toNumber(std::string_view str) {
    float sign = (str.front() == '-') ? -1 : 1;
    int value = 0;

    if (sign < 0) {
        str.remove_prefix(1);
    }

    auto it = str.begin();
    for (; it != str.end(); ++it) {
        if (*it == '.') {
            ++it;
            break;
        }
        value *= 10;
        value += (*it - '0');
    }

    float decimals = 0;

    for (; it != str.end(); ++it) {
        decimals += (*it - '0');
        decimals *= .1;
    }

    return std::copysign(decimals + value, sign);
}

struct Processor {
    struct Values {
        float sum;
        int size;
        float min = 1000;
        float max = -10000;

        void merge(const Values &other) {
            sum += other.sum;
            size += other.size;
            min = std::min(min, other.min);
            max = std::max(max, other.max);
        }
    };

    std::unordered_map<std::string, Values> stations;

    size_t tmpNum = 0; // Remove this

    void processLine(std::string_view line) {

        auto f = line.find(';');

        if (f == line.npos) {
            throw std::runtime_error{"no semicolon found"};
        }

        auto name = std::string{line.substr(0, f)};
        auto number = line.substr(f + 1);
        auto num = toNumber(number);
        ++tmpNum;
        if (tmpNum < 10) {
            std::cout << name << " = " << number << ", " << num << "\n";
        }

        auto &v = stations[name];
        v.sum += num;
        ++v.size;
        v.min = std::min(v.min, num);
        v.max = std::max(v.max, num);
    }

    void process(Split s) {
        auto &d = s.data;
        size_t lastLineStart = 0;
        size_t semiPos = 0;
        for (size_t i = 0; i < d.size(); ++i) {
            auto c = d[i];
            if (c == '\n') {
                auto line = std::string_view{d.data() + lastLineStart,
                                             i - lastLineStart};
                lastLineStart = i + 1;

                if (line.empty()) {
                    continue;
                }

                processLine(line);
            }
        }
    }
};

constexpr auto helpstr = "usage:\n./1brc <filename.txt>\n";

struct Settings {
    std::filesystem::path path;
    int numThreads = std::thread::hardware_concurrency();

    Settings(int argc, char *argv[]) {
        auto args = std::vector<std::string>{argv + 1, argv + argc};

        for (size_t i = 0; i < args.size(); ++i) {
            auto arg = args.at(i);

            if (arg == "--help" || arg == "-h") {
                std::cout << helpstr;
                std::exit(0);
            }
            if (arg == "--threads" || arg == "-j") {
                numThreads = std::stoi(args.at(++i));
            }
            else {
                path = arg;
            }
        }

        if (path.empty()) {
            std::cerr << helpstr << "\n";
            std::exit(-1);
        }
    }
};

int main(int argc, char *argv[]) {
    auto settings = Settings{argc, argv};

    std::cout << settings.path << std::endl;

    auto start = std::chrono::high_resolution_clock::now();

    auto file = std::ifstream{settings.path};

    if (!file) {
        std::cerr << "could not open file\n";
        std::terminate();
    }

    auto batches = 0;

    size_t fileLength = 0;

    auto stops = std::vector<size_t>{};

    // Split up files in chunks
    {
        // ate means to go for the end of the file
        std::ifstream file(settings.path, std::ios::binary | std::ios::ate);
        fileLength = file.tellg();

        std::cout << "file length " << fileLength << "\n";

        stops.resize(fileLength / batchSize);

        for (size_t i = 0; i < stops.size(); ++i) {
            stops.at(i) = i * fileLength / stops.size();
        }

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
        }

        stops.push_back(fileLength);
    }

    std::cout << "stops processed" << std::endl;

    auto ranges = [&stops] {
        auto ranges = std::vector<std::pair<size_t, size_t>>{};
        size_t lastPos = 0;
        for (auto stop : stops) {
            ranges.push_back({lastPos, stop});
            lastPos = stop;
        }
        return ranges;
    }();

    // Ranges that belongs to each thread
    auto threadRanges = std::vector<std::vector<std::pair<size_t, size_t>>>{};

    threadRanges.resize(settings.numThreads);

    for (size_t i = 0, t = 0; i < ranges.size();
         ++i, ++t, t = t % settings.numThreads) {
        threadRanges.at(t).push_back(ranges.at(i));
    }

    std::list<std::vector<std::pair<std::string, Processor::Values>>> stations;

    {
        auto threads = std::list<std::jthread>{};

        // Each thread is responsible for loading selected parts of the file
        for (auto &ranges : threadRanges) {
            threads.push_back(std::jthread{[ranges = std::move(ranges),
                                            &settings,
                                            &stations]() {
                auto processor = Processor{};
                std::cout << "thread " << std::this_thread::get_id() << "\n";

                auto file = std::ifstream{settings.path, std::ios::binary};

                for (auto range : ranges) {
                    file.seekg(range.first);
                    auto s = Split{};
                    s.data.resize(range.second - range.first);

                    file.read(s.data.data(), s.data.size());
                    processor.process(std::move(s));
                }

                std::cout << "num stations: " << processor.stations.size()
                          << std::endl;

                auto &s = stations.emplace_back(decltype(stations)::value_type(
                    processor.stations.begin(), processor.stations.end()));

                std::sort(s.begin(), s.end(), [](auto &a, auto &b) {
                    return a.first < b.first;
                });
                std::cout << "sort completed" << std::endl;
            }});
        }
    }

    auto result = std::vector<std::pair<std::string, Processor::Values>>{};

    result.resize(stations.front().size());

    for (auto &v : stations) {
        for (size_t i = 0; i < stations.front().size(); ++i) {
            result.at(i).second.merge(v.at(i).second);
        }
    }

    auto stop = std::chrono::high_resolution_clock::now();

    if (0) { // Change this to show the result
        for (auto &s : result) {
            std::cout << s.first << ": min:  " << s.second.min
                      << ", max: " << s.second.max
                      << ", mean: " << s.second.sum / s.second.size << "\n";
        }
    }

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
