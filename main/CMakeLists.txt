set(MAIN_DIR .)

file(GLOB_RECURSE MAIN_SOURCES ${MAIN_DIR}/*.c)

idf_component_register(
    SRCS
        ${MAIN_SOURCES}
    INCLUDE_DIRS
        ${MAIN_DIR}
)
