# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "UserInterface\\CMakeFiles\\UserInterface_autogen.dir\\AutogenUsed.txt"
  "UserInterface\\CMakeFiles\\UserInterface_autogen.dir\\ParseCache.txt"
  "UserInterface\\UserInterface_autogen"
  )
endif()
