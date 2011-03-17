cmake_minimum_required (VERSION 2.6)
project (libfatx)

# Setup the libfatx target

add_definitions(-Wall -Wextra -pedantic -Werror -std=c99)

add_library (fatx SHARED libfatx.c libfatx_internal.c)

install (TARGETS fatx LIBRARY DESTINATION lib)
install (FILES libfatx.h DESTINATION include)

# Setup Doxygen target
find_package(Doxygen)

configure_file(${CMAKE_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
add_custom_target(doxygen ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

set_property(DIRECTORY APPEND PROPERTY
             ADDITIONAL_MAKE_CLEAN_FILES doc)

get_target_property(DOC_TARGET doc TYPE)
if(NOT DOC_TARGET)
   add_custom_target(doc)
endif()

add_dependencies(doc doxygen)
