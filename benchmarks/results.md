## bm_chunked_stack

Linux / GCC 10 / Intel Core i5-4200U @ 1.60GHz

    Run on (4 X 2600 MHz CPU s)
    CPU Caches:
      L1 Data 32K (x2)
      L1 Instruction 32K (x2)
      L2 Unified 256K (x2)
      L3 Unified 3072K (x1)
    ---------------------------------------------------------------------
    Benchmark                              Time           CPU Iterations
    ---------------------------------------------------------------------
    bm_chunked_stack/8                    34 ns         34 ns   20545712
    bm_chunked_stack/64                  203 ns        203 ns    3597564
    bm_chunked_stack/512                1565 ns       1565 ns     452452
    bm_chunked_stack/4096              11834 ns      11831 ns      58712
    bm_chunked_stack/8192              23533 ns      23531 ns      29251
    bm_chunked_stack_reserve/8            34 ns         34 ns   21278951
    bm_chunked_stack_reserve/64          179 ns        179 ns    3930206
    bm_chunked_stack_reserve/512        1459 ns       1459 ns     433837
    bm_chunked_stack_reserve/4096      11910 ns      11905 ns      59276
    bm_chunked_stack_reserve/8192      23571 ns      23570 ns      30096
    bm_std_deque/8                        60 ns         60 ns   10748094
    bm_std_deque/64                      239 ns        239 ns    3041533
    bm_std_deque/512                    1641 ns       1641 ns     432623
    bm_std_deque/4096                  14061 ns      14060 ns      50403
    bm_std_deque/8192                  28812 ns      28809 ns      24605
    bm_chunked_stack_foreach/8             6 ns          6 ns  100107674
    bm_chunked_stack_foreach/64           45 ns         45 ns   15711439
    bm_chunked_stack_foreach/512         252 ns        252 ns    2710235
    bm_chunked_stack_foreach/4096       1675 ns       1675 ns     372127
    bm_chunked_stack_foreach/8192       3295 ns       3295 ns     204129
    bm_std_deque_foreach/8                 8 ns          8 ns   82854940
    bm_std_deque_foreach/64               51 ns         51 ns   13046052
    bm_std_deque_foreach/512             306 ns        304 ns    2307941
    bm_std_deque_foreach/4096           2459 ns       2456 ns     291601
    bm_std_deque_foreach/8192           4832 ns       4827 ns     139442
    bm_chunked_stack_pump/8              559 ns        559 ns    1208602
    bm_chunked_stack_pump/64            4321 ns       4317 ns     160040
    bm_chunked_stack_pump/512          34302 ns      34268 ns      20980
    bm_chunked_stack_pump/4096        271474 ns     271171 ns       2646
    bm_chunked_stack_pump/8192        530942 ns     530875 ns       1261
    bm_std_deque_pump/8                  640 ns        640 ns    1093669
    bm_std_deque_pump/64                4248 ns       4247 ns     166817
    bm_std_deque_pump/512              33567 ns      33565 ns      20034
    bm_std_deque_pump/4096            270216 ns     270191 ns       2454
    bm_std_deque_pump/8192            559680 ns     558439 ns       1234

MacOS / Clang 10 / Intel Core i7 @ 2,2 GHz

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
