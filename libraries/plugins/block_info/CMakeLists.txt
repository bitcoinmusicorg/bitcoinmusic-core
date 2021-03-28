file(GLOB HEADERS "include/btcm/plugins/block_info/*.hpp")

add_library( btcm_block_info
             ${HEADERS}
             block_info_plugin.cpp
             block_info_api.cpp
           )

target_link_libraries( btcm_block_info btcm_app btcm_chain fc graphene_db )
target_include_directories( btcm_block_info
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
