# Standalone unit tests for the word-matching logic (see test_jumpword.cpp for why
# these are duplicated copies rather than linking against src/JumpWord.cpp).
#
# Build directly, independent of the far2l tree, e.g.:
#   c++ -std=c++17 -o test_jumpword test_jumpword.cpp && ./test_jumpword
#
# Some tests are currently expected to fail -- they document known bugs.

cmake_minimum_required(VERSION 3.10)
project(jumpword_tests CXX)

add_executable(jumpword_tests test_jumpword.cpp)
set_target_properties(jumpword_tests PROPERTIES CXX_STANDARD 17)

enable_testing()
add_test(NAME jumpword_tests COMMAND jumpword_tests)
