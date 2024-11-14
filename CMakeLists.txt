# Minimal version of CMake
cmake_minimum_required (VERSION 2.8...3.27)

# Build type
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	message(STATUS "Setting build type to 'Release' as none was specified.")
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
	# Set the possible values of build type for cmake-gui
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
		"MinSizeRel" "RelWithDebInfo")
endif ()

project (InventoryHIPA daemon)

set (CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/build_files/cmake/modules/")

set (EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin/)
add_subdirectory (src)

set (INVENTORYD_CONF_DIR "/etc/op_inventoryd")
set (SYSTEMD_UNIT_DIR "/usr/lib/systemd/system/")
set (INVENTORYD_LOG_DIR "/var/log/op_inventoryd")
set (INVENTORYD_PID_DIR "/run/op_inventoryd")

# Macro for installing configuration files
function(install_conf src dest)
  if(NOT IS_ABSOLUTE "${src}")
    set(src "${CMAKE_CURRENT_SOURCE_DIR}/${src}")
  endif()
  get_filename_component(src_name "${src}" NAME)
  if (NOT IS_ABSOLUTE "${dest}")
    set(dest "${CMAKE_INSTALL_PREFIX}/${dest}")
  endif()
  install(CODE "
    if(NOT EXISTS \"\$ENV{DESTDIR}${dest}/${src_name}\")
      #file(INSTALL \"${src}\" DESTINATION \"${dest}\")
      message(STATUS \"Installing: \$ENV{DESTDIR}${dest}/${src_name}\")
      execute_process(COMMAND \${CMAKE_COMMAND} -E copy \"${src}\"
                      \"\$ENV{DESTDIR}${dest}/${src_name}\"
                      RESULT_VARIABLE copy_result
                      ERROR_VARIABLE error_output)
      if(copy_result)
        message(FATAL_ERROR \${error_output})
      endif()
    else()
      message(STATUS \"Skipping  : \$ENV{DESTDIR}${dest}/${src_name}\")
    endif()
  ")
endfunction(install_conf)


install_conf (./op_inventoryd.conf ${INVENTORYD_CONF_DIR})
install_conf (./op_inventoryd.service ${SYSTEMD_UNIT_DIR})
install(DIRECTORY DESTINATION ${INVENTORYD_LOG_DIR})
install(DIRECTORY DESTINATION ${INVENTORY_PID_DIR})
