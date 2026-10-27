add_library(storage EXCLUDE_FROM_ALL
    storage.cc
)

target_include_directories(storage
PUBLIC
    include
)

target_link_libraries(storage
PUBLIC
    application_state
    job_pool_thread
    route_service
)
