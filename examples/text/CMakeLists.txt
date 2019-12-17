add_executable(demo_font demo_font.cpp)
target_link_libraries(demo_font xci-text)
install(TARGETS demo_font EXPORT xcikit DESTINATION ${INST_EXAMPLES})

add_executable(demo_layout demo_layout.cpp)
target_link_libraries(demo_layout xci-text)
install(TARGETS demo_layout EXPORT xcikit DESTINATION ${INST_EXAMPLES})

add_executable(demo_glyph_cluster demo_glyph_cluster.cpp)
target_link_libraries(demo_glyph_cluster xci-text)
install(TARGETS demo_glyph_cluster EXPORT xcikit DESTINATION ${INST_EXAMPLES})
