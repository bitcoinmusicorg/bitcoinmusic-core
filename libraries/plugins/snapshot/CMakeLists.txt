file(GLOB HEADERS "include/graphene/snapshot/*.hpp")

add_library( btcm_snapshot
             snapshot.cpp
           )

target_link_libraries( btcm_snapshot btcm_chain btcm_app )
target_include_directories( btcm_snapshot
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   btcm_snapshot

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
