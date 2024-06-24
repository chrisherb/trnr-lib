project(trnr-lib)

add_library(trnr-lib INTERFACE)

# Add include directories
target_include_directories(trnr-lib INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/clip
    ${CMAKE_CURRENT_SOURCE_DIR}/companding
    ${CMAKE_CURRENT_SOURCE_DIR}/dynamics
    ${CMAKE_CURRENT_SOURCE_DIR}/filter
    ${CMAKE_CURRENT_SOURCE_DIR}/oversampling
    ${CMAKE_CURRENT_SOURCE_DIR}/synth
    ${CMAKE_CURRENT_SOURCE_DIR}/util
)
