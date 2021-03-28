file(GLOB HEADERS "include/btcm/plugins/raw_block/*.hpp")

add_library( btcm_raw_block
             ${HEADERS}
             raw_block_plugin.cpp
             raw_block_api.cpp
           )

target_link_libraries( btcm_raw_block btcm_app btcm_chain fc graphene_db )
target_include_directories( btcm_raw_block
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
