cmake_minimum_required(VERSION 3.22)

# Enable IDE organization with folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Add third-party dependencies
add_subdirectory("third")

# Add first-party dependencies
add_subdirectory("first")
