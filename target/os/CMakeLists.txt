add_library(target_os EXCLUDE_FROM_ALL
    base_thread_freertos.cc
    semaphore_freertos.cc
)

target_link_libraries(target_os
PUBLIC
    idf::freertos
    maelir_interface
)
