# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\HillClimbRacing_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\HillClimbRacing_autogen.dir\\ParseCache.txt"
  "HillClimbRacing_autogen"
  )
endif()
