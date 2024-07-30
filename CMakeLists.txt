cmake_minimum_required(VERSION 3.14)

project(FinalProject LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network)

# MetadataNode executable
add_executable(MetadataNode
  MetadataNode.cpp
  MetadataNode.h
)

# AnalyticsNode executable
add_executable(AnalyticsNode
  AnalyticsNode.cpp
  AnalyticsNode.h
  Worker.h
  Worker.cpp
)

#RegisterNode executable
add_executable(RegisterNode
    RegisterNode.cpp
    RegisterNode.h
)

#Demo ingestion node
add_executable(DemoIngestionNode
    DemoIngestionNode.cpp
    DemoIngestionNode.h
)

# Linking Qt libraries with MetadataNode
target_link_libraries(MetadataNode Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Network)

# Linking Qt libraries with AnalyticsNode
target_link_libraries(AnalyticsNode Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Network)

# Linking Qt libraries with RegisterNode
target_link_libraries(RegisterNode Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Network)

# Linking Qt libraries with DemoIngestionNode
target_link_libraries(DemoIngestionNode Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Network)


include(GNUInstallDirs)
install(TARGETS MetadataNode AnalyticsNode DemoIngestionNode RegisterNode
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
