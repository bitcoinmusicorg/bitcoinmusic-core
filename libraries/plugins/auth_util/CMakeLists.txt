file(GLOB HEADERS "include/btcm/plugins/auth_util/*.hpp")

add_library( btcm_auth_util
             ${HEADERS}
             auth_util_plugin.cpp
             auth_util_api.cpp
           )

target_link_libraries( btcm_auth_util btcm_app btcm_chain fc graphene_db )
target_include_directories( btcm_auth_util
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
