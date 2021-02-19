file(GLOB HEADERS "include/btcm/account_history/*.hpp")

add_library( btcm_account_history
             account_history_plugin.cpp
           )

target_link_libraries( btcm_account_history btcm_chain btcm_app )
target_include_directories( btcm_account_history
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   btcm_account_history

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
