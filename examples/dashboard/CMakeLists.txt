add_executable(dashboard main.cpp)
target_link_libraries(dashboard PRIVATE Tavoos)
tavoos_add_resources(dashboard
	BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..
	assets/fonts/Inter-Regular.ttf
	assets/fonts/Inter-Medium.ttf
	assets/fonts/Inter-SemiBold.ttf
	assets/fonts/Inter-Bold.ttf
)
tavoos_enable_warnings(dashboard)
