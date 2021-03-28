add_executable( btcmd main.cpp )
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling btcmd with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

target_link_libraries( btcmd
                       PRIVATE btcm_external_plugins btcm_internal_plugins btcm_mf_plugins btcm_app btcm_witness btcm_account_history btcm_chain btcm_egenesis_full btcm_snapshot fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )

install( TARGETS
        btcmd

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
