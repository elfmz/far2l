if (NOT CMAKE_BASH_COMP_DIR)
  set(CMAKE_BASH_COMP_DIR ${CMAKE_XDGDATA_DIR}/bash-completion/completions)
endif()
install(FILES far2l DESTINATION ${CMAKE_BASH_COMP_DIR})
