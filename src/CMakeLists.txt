include(../extern/guikit/cmake/Sanitizers.cmake)
include(../extern/guikit/cmake/Coverage.cmake)
include(../extern/guikit/cmake/Warnings.cmake)

add_executable(shaderkit
    main.c
)

find_package(SDL2 REQUIRED)
target_link_libraries(shaderkit PUBLIC
    ${SDL2_LIBRARIES}
)
target_include_directories(shaderkit SYSTEM PRIVATE
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(shaderkit PUBLIC
    glad
)

target_compile_features(shaderkit PUBLIC c_std_90)

enable_sanitizers(shaderkit)
enable_coverage(shaderkit)
enable_warnings(shaderkit)
