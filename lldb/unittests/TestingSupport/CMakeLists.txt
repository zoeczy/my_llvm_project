set_property(DIRECTORY PROPERTY EXCLUDE_FROM_ALL ON)
add_lldb_library(lldbUtilityHelpers
  MockTildeExpressionResolver.cpp
  TestUtilities.cpp

  LINK_LIBS
    lldbUtility
    llvm_gtest

  LINK_COMPONENTS
    Support
    ObjectYAML
  )

add_subdirectory(Host)
add_subdirectory(Symbol)
