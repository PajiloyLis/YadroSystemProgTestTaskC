cmake_minimum_required(VERSION 4.1)
project(YadroSystemProg C)

set(CMAKE_C_STANDARD 17)

add_executable(YadroSystemProg main.c)

target_compile_options(YadroSystemProg PRIVATE
        -Wall
        -Wextra
        -Wvla
#        -fsanitize=address,undefined
#        -fno-omit-frame-pointer
#        -g
        -O2
        -std=c17
)

target_link_options(YadroSystemProg PRIVATE
        -fsanitize=address,undefined
)
