file(GLOB HEADERS "include/btcm/market_history/*.hpp")

add_library( btcm_market_history
             market_history_plugin.cpp
             market_history_api.cpp
           )

target_link_libraries( btcm_market_history btcm_chain btcm_app )
target_include_directories( btcm_market_history
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   btcm_market_history

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)