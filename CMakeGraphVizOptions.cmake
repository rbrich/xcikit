set(GRAPHVIZ_CUSTOM_TARGETS ON)

# Don't generate sub-graphs
set(GRAPHVIZ_GENERATE_PER_TARGET OFF)
set(GRAPHVIZ_GENERATE_DEPENDERS OFF)

# Ignore tests and demos
set(GRAPHVIZ_IGNORE_TARGETS demo_.* test_.* bm_.* catch_main benchmark::benchmark Catch2::Catch2)
