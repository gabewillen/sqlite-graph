# CTest Configuration File
# This allows running ctest from the source directory

set(CTEST_PROJECT_NAME "SQLiteGraph")
set(CTEST_NIGHTLY_START_TIME "00:00:00 EST")

set(CTEST_DROP_METHOD "http")
set(CTEST_DROP_SITE "")
set(CTEST_DROP_LOCATION "")
set(CTEST_DROP_SITE_CDASH FALSE)

# Set the build directory
set(CTEST_BINARY_DIRECTORY "/shared/sqlite-graph/build")
set(CTEST_SOURCE_DIRECTORY "/shared/sqlite-graph")

# Configure to run from source directory
set(CTEST_CMAKE_GENERATOR "Unix Makefiles")
