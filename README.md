# 1brc

## 

This is a c++ solution for
[1brc](https://github.com/gunnarmorling/1brc?tab=readme-ov-file). In this 
challenge you are supposed to read a file with unordered weather data, parse
and do some calculations on it.

My solution is fast enough for me, I did not have the motivation to benchmark
against other solutions.

## Method

My theory was that loading the file was going to be the bottleneck.

After changing to multithreaded file loading i lowered the loading time from around 
`1238ms` to around `900ms`.

Sorting the result is what takes up the majority of the rest of the time.

I have one unordered_map per thread. Those then get merged in the end in the main
thread (that seems to be fast).

## Improvements

If I would like to do some improvements it would be to replace std::unordered_map
with something faster.

## result

I landed on around `3775ms`
on my AMD Ryzen 9 5950X 16-Core Processor with 32 threads.

It's fast enogh for me to stop thinking about the problem any more.
