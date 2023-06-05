cmake_minimum_required(VERSION 3.25)
project(PublicadorSubscriptor C)

set(CMAKE_C_STANDARD 11)

add_executable(PublicadorSubscriptor main.c broker.c proxy/proxy.c publisher.c subscriber.c proxy/proxy.h process/process.c)
