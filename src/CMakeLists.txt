set(TAVOOS_SOURCES
    application.cpp
    object.cpp
    window.cpp
    builder.cpp
    animation/animationmanager.cpp
    gfx/renderer.cpp
    gfx/shader.cpp
    gfx/texturecache.cpp
    gfx/svgdocumentcache.cpp
    gfx/svgtexturecache.cpp
    widget/widget.cpp
    widget/rectangle.cpp
    widget/row.cpp
    widget/column.cpp
    widget/grid.cpp
    widget/flex.cpp
    widget/image.cpp
    widget/svg.cpp
    widget/text.cpp
    resources/resourceregistry.cpp
    resources/pathscheme.cpp
    text/fontface.cpp
    text/fontfamily.cpp
    text/fontmanager.cpp
    text/systemfont.cpp
    text/glyphatlas.cpp
    text/textlayout.cpp
    third_party/glad/glad.c
    third_party/stb_image/stb_image_impl.cpp
)

add_library(Tavoos SHARED ${TAVOOS_SOURCES})

target_sources(Tavoos
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
    FILES
    include/tavoos/application.h
    include/tavoos/object.h
    include/tavoos/window.h
    include/tavoos/builder.h
    include/tavoos/types.h
    include/tavoos/widget/widget.h
    include/tavoos/widget/rectangle.h
    include/tavoos/widget/row.h
    include/tavoos/widget/column.h
    include/tavoos/widget/grid.h
    include/tavoos/widget/flex.h
    include/tavoos/widget/image.h
    include/tavoos/widget/svg.h
    include/tavoos/widget/text.h
    include/tavoos/widget/widgets.h
    include/tavoos/gfx/renderer.h
    include/tavoos/gfx/shader.h
    include/tavoos/gfx/shader_sources.h
    include/tavoos/gfx/texturecache.h
    include/tavoos/gfx/svgdocumentcache.h
    include/tavoos/gfx/svgtexturecache.h
    include/tavoos/gfx/gfx.h
    include/tavoos/reactive/state.h
    include/tavoos/reactive/property.h
    include/tavoos/reactive/propertyarg.h
    include/tavoos/reactive/sidedproperty.h
    include/tavoos/reactive/cornerproperty.h
    include/tavoos/reactive/bindablestate.h
    include/tavoos/reactive/reactive.h
    include/tavoos/events/event.h
    include/tavoos/events/mouseevent.h
    include/tavoos/events/wheelevent.h
    include/tavoos/events/keyevent.h
    include/tavoos/events/key.h
    include/tavoos/events/events.h
    include/tavoos/animation/easing.h
    include/tavoos/animation/animatedstate.h
    include/tavoos/animation/animationmanager.h
    include/tavoos/animation/animation.h
    include/tavoos/resources/resourceregistry.h
    include/tavoos/resources/pathscheme.h
    include/tavoos/resources/resources.h
    include/tavoos/text/font.h
    include/tavoos/text/fontenums.h
    include/tavoos/text/glyphmetrics.h
    include/tavoos/text/fontface.h
    include/tavoos/text/fontfamily.h
    include/tavoos/text/fontmanager.h
    include/tavoos/text/glyphatlas.h
    include/tavoos/text/utf8.h
    include/tavoos/text/textlayout.h
    include/tavoos/text/text.h
    include/tavoos/third_party/glad/glad.h
    include/tavoos/third_party/KHR/khrplatform.h
)

target_compile_features(Tavoos PUBLIC cxx_std_23)

set_target_properties(Tavoos PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

include(GenerateExportHeader)
generate_export_header(Tavoos
    BASE_NAME tavoos
    EXPORT_FILE_NAME include/tavoos/export.hpp
)

target_sources(Tavoos
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_BINARY_DIR}/include
    FILES
    ${CMAKE_CURRENT_BINARY_DIR}/include/tavoos/export.hpp
)

set_target_properties(Tavoos PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

target_include_directories(Tavoos PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include/tavoos/third_party
)

target_link_libraries(Tavoos
    PUBLIC
    glm::glm
    OpenGL::GL
    Freetype::Freetype
    PRIVATE
    glfw
    spdlog::spdlog
    lunasvg::lunasvg
)

if (UNIX AND NOT APPLE)
    target_link_libraries(Tavoos PRIVATE Fontconfig::Fontconfig)
    target_compile_definitions(Tavoos PRIVATE TAVOOS_HAVE_FONTCONFIG)
endif()

tavoos_enable_warnings(Tavoos)

install(
    TARGETS Tavoos
    EXPORT TavoosTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
