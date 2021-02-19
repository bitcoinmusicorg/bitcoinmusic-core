file(GLOB HEADERS "include/btcm/private_message/*.hpp")

add_library( btcm_private_message
             private_message_plugin.cpp
           )

target_link_libraries( btcm_private_message btcm_chain btcm_app )
target_include_directories( btcm_private_message
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   btcm_private_message

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
