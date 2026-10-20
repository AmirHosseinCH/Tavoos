add_executable(focus main.cpp)
target_link_libraries(focus PRIVATE Tavoos)
tavoos_add_resources(focus
	BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..
	assets/fonts/Inter-Light.ttf
	assets/fonts/Inter-Regular.ttf
	assets/fonts/Inter-Medium.ttf
	assets/fonts/Inter-SemiBold.ttf
)
tavoos_enable_warnings(focus)
