#pragma once

#include "batchsplitter.h"
#include <cmath>
#include <map>
#include <stdexcept>
#include <string>
#include <unordered_map>

inline float toNumber(std::string_view str) {
    float sign = (str.front() == '-') ? -1 : 1;
    int value = 0;

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

    return decimals + std::copysign(value, sign);
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

    std::list<Split> queue;

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
            // if (c == ';') {
            //     semiPos = i;

            // }
        }
    }
};
