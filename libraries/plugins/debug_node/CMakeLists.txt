file(GLOB HEADERS "include/btcm/plugins/debug_node/*.hpp")

add_library( btcm_debug_node
             ${HEADERS}
             debug_node_plugin.cpp
             debug_node_api.cpp
           )

target_link_libraries( btcm_debug_node btcm_app btcm_chain fc graphene_db )
target_include_directories( btcm_debug_node
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
