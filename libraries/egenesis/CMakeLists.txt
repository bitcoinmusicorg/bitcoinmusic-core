message( STATUS "Generating egenesis" )

if( GRAPHENE_EGENESIS_JSON )
  set( embed_genesis_args "${GRAPHENE_EGENESIS_JSON}" )
else( GRAPHENE_EGENESIS_JSON )
  set( embed_genesis_args "${CMAKE_CURRENT_SOURCE_DIR}/genesis.json" )
endif( GRAPHENE_EGENESIS_JSON )

FOREACH( _type brief full )
  add_custom_command(
    OUTPUT
      "${CMAKE_CURRENT_BINARY_DIR}/egenesis_${_type}.cpp"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND}
        -DINIT_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}
        -DINIT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
        -Dembed_genesis_args=${embed_genesis_args}
        -Dembed_genesis_type=${_type}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/embed_genesis.cmake
    DEPENDS
      "${embed_genesis_args}"
      "${CMAKE_CURRENT_SOURCE_DIR}/egenesis_${_type}.cpp.tmpl"
  )
ENDFOREACH( _type )

add_library( btcm_egenesis_none egenesis_none.cpp
             include/btcm/egenesis/egenesis.hpp )
add_library( btcm_egenesis_brief "${CMAKE_CURRENT_BINARY_DIR}/egenesis_brief.cpp"
             include/btcm/egenesis/egenesis.hpp )
add_library( btcm_egenesis_full  "${CMAKE_CURRENT_BINARY_DIR}/egenesis_full.cpp"
             include/btcm/egenesis/egenesis.hpp )

target_link_libraries(btcm_egenesis_none  btcm_chain fc)
target_link_libraries(btcm_egenesis_brief btcm_chain fc)
target_link_libraries(btcm_egenesis_full  btcm_chain fc)

target_include_directories( btcm_egenesis_none
   PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
target_include_directories( btcm_egenesis_brief
   PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
target_include_directories( btcm_egenesis_full
   PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

INSTALL( TARGETS
   btcm_egenesis_none btcm_egenesis_brief btcm_egenesis_full

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
