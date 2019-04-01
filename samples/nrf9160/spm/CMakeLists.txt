cmake_minimum_required(VERSION 3.8.2)

zephyr_add_executable(spm require_build)

if (${require_build})
  include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
  project(NONE)

  target_sources(${IMAGE}app PRIVATE src/main.c)
endif ()
