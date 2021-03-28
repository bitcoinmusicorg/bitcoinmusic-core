file(GLOB HEADERS "include/btcm/app/*.hpp")
file(GLOB EGENESIS_HEADERS "../egenesis/include/btcm/egenesis/*.hpp")

add_library( btcm_app
             database_api.cpp
             api.cpp
             application.cpp
             impacted.cpp
             plugin.cpp
             ${HEADERS}
             ${EGENESIS_HEADERS}
           )

target_link_libraries( btcm_app btcm_chain  btcm_mf_plugins fc graphene_db graphene_net graphene_utilities )
target_include_directories( btcm_app
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" 
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../egenesis/include")

if(MSVC)
  set_source_files_properties( application.cpp api.cpp database_api.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

INSTALL( TARGETS
   btcm_app

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
