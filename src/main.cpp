#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
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

    auto file = std::ifstream{arg.path};

    for (std::string line; std::getline(file, line);) {
    }

    auto stop = std::chrono::high_resolution_clock::now();

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
