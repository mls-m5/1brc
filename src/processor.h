#pragma once

#include "batchsplitter.h"
#include <stdexcept>

struct Processor {
    std::list<Split> queue;

    void processLine(std::string_view line) {
        // std::cout << line << "\n";

        auto f = line.find(';');

        if (f == line.npos) {
            throw std::runtime_error{"no semicolon found"};
        }

        auto name = line.substr(0, f);
        auto number = line.substr(f + 1);

        // std::cout << name << ", " << number << std::endl;
    }

    void process(Split s) {
        auto &d = s.data;
        size_t lastLineStart = 0;
        for (size_t i = 0; i < d.size(); ++i) {
            if (d[i] == '\n') {
                auto line = std::string_view{d.data() + lastLineStart,
                                             i - lastLineStart};
                lastLineStart = i + 1;

                processLine(line);
            }
        }
    }
};
