cmake_minimum_required(VERSION 3.22)
project(PublicadorSubscriptor C)

set(CMAKE_C_STANDARD 11)

add_executable(PublicadorSubscriptor broker.c proxy/proxy.c publisher.c subscriber.c proxy/proxy.h process/process.c process/process.h proxynew.cpp proxy/newProxy.c proxy/newProxy.h)
