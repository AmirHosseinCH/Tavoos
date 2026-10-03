add_executable(loginForm main.cpp)
target_link_libraries(loginForm PRIVATE Tavoos)
tavoos_add_resources(loginForm
	assets/fonts/Inter-Regular.ttf
)
tavoos_enable_warnings(loginForm)
