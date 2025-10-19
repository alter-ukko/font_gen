cmake_minimum_required(VERSION 3.3...4.0)
project(font_gen C)

set(CMAKE_C_STANDARD 99)

add_executable(font_gen main.c misc_util.c argparse.c)
