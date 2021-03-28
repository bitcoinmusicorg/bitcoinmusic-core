file(GLOB HEADERS "include/btcm/custom_tags/*.hpp")

add_library( btcm_custom_tags
             custom_tags.cpp
             custom_tags_api.cpp
           )

target_link_libraries( btcm_custom_tags btcm_chain btcm_app )
target_include_directories( btcm_custom_tags
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   btcm_custom_tags

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
