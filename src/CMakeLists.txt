set (inventoryd_src op_inventoryd.c)

# Make build flags compiler specific
if (CMAKE_COMPILER_IS_GNUCC)
	if (CMAKE_BUILD_TYPE STREQUAL "Debug")
		set (CMAKE_C_FLAGS "-D_REETRANT -ggdb -D__NO_USE_ANNOBIN -fPIC -Wall -Wextra -pedantic -O0")
	elseif( CMAKE_BUILD_TYPE STREQUAL "Release" )
		set (CMAKE_C_FLAGS "-D_REETRANT  -D__NO_USE_ANNOBIN -DNDEBUG -fPIC -Wall -Wextra -pedantic -O3")
	endif ()
endif (CMAKE_COMPILER_IS_GNUCC)

add_executable (op_inventoryd ${inventoryd_src})

# The rule to install daemon binary
install (TARGETS op_inventoryd
	RUNTIME
	DESTINATION bin)
