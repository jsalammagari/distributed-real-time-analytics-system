# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "AnalyticsNode_autogen"
  "CMakeFiles\\AnalyticsNode_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\AnalyticsNode_autogen.dir\\ParseCache.txt"
  "CMakeFiles\\DemoIngestionNode_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\DemoIngestionNode_autogen.dir\\ParseCache.txt"
  "CMakeFiles\\MetadataNode_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\MetadataNode_autogen.dir\\ParseCache.txt"
  "CMakeFiles\\RegisterNode_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\RegisterNode_autogen.dir\\ParseCache.txt"
  "DemoIngestionNode_autogen"
  "MetadataNode_autogen"
  "RegisterNode_autogen"
  )
endif()
