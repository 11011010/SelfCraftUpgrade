AC_ADD_SCRIPT("${CMAKE_CURRENT_LIST_DIR}/src/SelfCraftUpgrade.cpp")

if( WIN32 )
  if ( MSVC )
    add_custom_command(TARGET worldserver
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/conf/SelfCraftUpgrade.conf.dist" ${CMAKE_BINARY_DIR}/bin/$(ConfigurationName)/
    )
  elseif ( MINGW )
    add_custom_command(TARGET worldserver
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_LIST_DIR}/conf/SelfCraftUpgrade.conf.dist" ${CMAKE_BINARY_DIR}/bin/
    )
  endif()
endif()

install(FILES "${CMAKE_CURRENT_LIST_DIR}/conf/SelfCraftUpgrade.conf.dist" DESTINATION ${CONF_DIR})
