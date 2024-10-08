cmake_minimum_required(VERSION 3.10)
project(HelloCMake)

# Указываем стандарт C++
set(CMAKE_CXX_STANDARD 17)

# Добавляем исполняемый файл
add_executable(HelloCMake main.cpp)
