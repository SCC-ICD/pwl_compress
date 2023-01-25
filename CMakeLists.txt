# To build and run from command line:
#
# 1. setup build: cmake -S . -B build
# 2. build      : cmake --build build --config Debug
# 4. run        : .\build\Debug\pwl_codec_test.exe

project(pwl_compress)

add_executable(
  pwl_codec_test
  pwl_codec.c
  pwl_codec_test.c
)
