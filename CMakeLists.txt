cmake_minimum_required(VERSION 3.14)

project(mmisim)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "-g")

set(SOURCE_FILES
    BasicLogic.c
    Utils.c
    strem_structures/strem_vector.c
    strem_structures/strem_ht.c
    strem_structures/strem_queue.c
)
set(INCLUDE_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/strem_structures/"
    "${CMAKE_CURRENT_LIST_DIR}"
)
message(
    "${INCLUDE_DIRS}"
    )

add_library(mmisim STATIC "${SOURCE_FILES}")
# target_link_libraries()
target_include_directories(mmisim PUBLIC "${INCLUDE_DIRS}")
