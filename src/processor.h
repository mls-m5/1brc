#pragma once

#include "batchsplitter.h"
#include <stdexcept>

struct Processor {
    std::list<Split> queue;

    size_t tmpNum = 0; // Remove this

    // struct Pair {
    //     std::string_view name;
    //     std::string_view value;
    // };

    void processLine(std::string_view line) {
        // ++tmpNum;
        // if (tmpNum < 10) {
        //     std::cout << line << "\n";
        // }

        auto f = line.find(';');

        if (f == line.npos) {
            throw std::runtime_error{"no semicolon found"};
        }

        auto name = line.substr(0, f);
        auto number = line.substr(f + 1);
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
            // if (c == ';') {
            //     semiPos = i;

            // }
        }
    }
};
