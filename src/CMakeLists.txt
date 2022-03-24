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
