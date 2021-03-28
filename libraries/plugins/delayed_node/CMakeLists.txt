file(GLOB HEADERS "include/btcm/delayed_node/*.hpp")

add_library( btcm_delayed_node
             delayed_node_plugin.cpp
           )

target_link_libraries( btcm_delayed_node btcm_chain btcm_app )
target_include_directories( btcm_delayed_node
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if(MSVC)
  set_source_files_properties( delayed_node_plugin.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

install( TARGETS
   btcm_delayed_node

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
