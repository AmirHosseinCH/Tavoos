add_executable(tavoos_example main.cpp)
target_link_libraries(tavoos_example PRIVATE Tavoos)
tavoos_add_resources(tavoos_example
	assets/fonts/Inter-Regular.ttf
)
tavoos_enable_warnings(tavoos_example)
