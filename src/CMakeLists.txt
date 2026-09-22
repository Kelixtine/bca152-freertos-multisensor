FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

idf_component_register(
    SRCS "main.cpp" "rtos_objects.cpp" "system_state.cpp" "sensors.cpp" "input.cpp" "display.cpp" "alarm.cpp" "motion.cpp"
    INCLUDE_DIRS "../include"
    REQUIRES driver esp_adc
)