file(GLOB HEADERS "include/btcm/witness/*.hpp")

add_library( btcm_witness
             witness.cpp
           )

target_link_libraries( btcm_witness btcm_chain btcm_app )
target_include_directories( btcm_witness
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   btcm_witness

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
