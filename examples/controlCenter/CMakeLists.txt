add_executable(controlCenter main.cpp)
target_link_libraries(controlCenter PRIVATE Tavoos)
tavoos_add_resources(controlCenter
	BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..
	assets/fonts/Inter-Regular.ttf
	assets/fonts/Inter-Medium.ttf
	assets/fonts/Inter-SemiBold.ttf
	assets/fonts/Inter-Bold.ttf
)
tavoos_enable_warnings(controlCenter)
