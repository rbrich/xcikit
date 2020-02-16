add_executable(demo_widgets demo_widgets.cpp)
target_link_libraries(demo_widgets xci-widgets xci-text xci-graphics)
install(TARGETS demo_widgets EXPORT xcikit DESTINATION examples/widgets)

add_executable(demo_ui demo_ui.cpp)
target_link_libraries(demo_ui xci-widgets xci-text xci-graphics)
install(TARGETS demo_ui EXPORT xcikit DESTINATION examples/widgets)

add_executable(demo_form demo_form.cpp)
target_link_libraries(demo_form xci-widgets xci-text xci-graphics)
install(TARGETS demo_form EXPORT xcikit DESTINATION examples/widgets)

add_executable(demo_term_widget demo_term_widget.cpp)
target_link_libraries(demo_term_widget xci-widgets xci-graphics)
install(TARGETS demo_term_widget EXPORT xcikit DESTINATION examples/widgets)

add_executable(demo_term_dispatch demo_term_dispatch.cpp)
target_link_libraries(demo_term_dispatch xci-widgets xci-graphics)
install(TARGETS demo_term_dispatch EXPORT xcikit DESTINATION examples/widgets)
