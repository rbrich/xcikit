## bm_chunked_stack

MacOS - Intel Core i7, 2,2 GHz

    Run on (8 X 2200 MHz CPU s)
    CPU Caches:
      L1 Data 32K (x4)
      L1 Instruction 32K (x4)
      L2 Unified 262K (x4)
      L3 Unified 6291K (x1)
    Load Average: 2.36, 2.04, 2.08
    ------------------------------------------------------------------------
    Benchmark                              Time             CPU   Iterations
    ------------------------------------------------------------------------
    bm_chunked_stack/8                   127 ns          109 ns      7197795
    bm_chunked_stack/64                  343 ns          341 ns      2016170
    bm_chunked_stack/512                1410 ns         1406 ns       506271
    bm_chunked_stack/4096               8240 ns         8224 ns        81640
    bm_chunked_stack/8192              16194 ns        16164 ns        43258
    bm_chunked_stack_reserve/8          94.2 ns         94.1 ns      7555968
    bm_chunked_stack_reserve/64          275 ns          275 ns      2570902
    bm_chunked_stack_reserve/512        1055 ns         1053 ns       667945
    bm_chunked_stack_reserve/4096       7801 ns         7788 ns        91423
    bm_chunked_stack_reserve/8192      16177 ns        16138 ns        43533
    bm_std_deque/8                       202 ns          202 ns      3589670
    bm_std_deque/64                      330 ns          330 ns      2108618
    bm_std_deque/512                    1215 ns         1213 ns       574326
    bm_std_deque/4096                   9531 ns         9516 ns        75052
    bm_std_deque/8192                  18341 ns        18305 ns        38614
    bm_chunked_stack_foreach/8          5.50 ns         5.49 ns    131388779
    bm_chunked_stack_foreach/64         38.7 ns         38.6 ns     17719141
    bm_chunked_stack_foreach/512         279 ns          279 ns      2473516
    bm_chunked_stack_foreach/4096       2065 ns         2062 ns       337952
    bm_chunked_stack_foreach/8192       4217 ns         4208 ns       164145
    bm_std_deque_foreach/8              7.64 ns         7.63 ns     87293768
    bm_std_deque_foreach/64             45.9 ns         45.9 ns     15164152
    bm_std_deque_foreach/512             289 ns          289 ns      2379124
    bm_std_deque_foreach/4096           2280 ns         2277 ns       305003
    bm_std_deque_foreach/8192           4519 ns         4514 ns       152406
    bm_chunked_stack_pump/8              554 ns          553 ns      1262421
    bm_chunked_stack_pump/64            3939 ns         3933 ns       178064
    bm_chunked_stack_pump/512          27839 ns        27808 ns        24925
    bm_chunked_stack_pump/4096        213545 ns       213125 ns         3229
    bm_chunked_stack_pump/8192        429856 ns       428873 ns         1637
    bm_std_deque_pump/8                  720 ns          718 ns       963471
    bm_std_deque_pump/64                3895 ns         3891 ns       179372
    bm_std_deque_pump/512              29480 ns        29439 ns        23549
    bm_std_deque_pump/4096            235580 ns       235226 ns         2936
    bm_std_deque_pump/8192            469644 ns       469182 ns         1488
