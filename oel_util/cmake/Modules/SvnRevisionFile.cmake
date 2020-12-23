# 
#
# svn_revision_file

include(CMakeParseArguments)

function(svn_revision_file)
	unset(SRF_VERSION)
	
	set(options PRINT READ WRITE UPDATE INFO)
	set(oneValueArgs URL NAME FILE DIR REPO OUTPUT_VARIABLE VERSION GENERATE CHECKOUT_CMD COMMIT_CMD)
	set(multiValueArgs)
	cmake_parse_arguments(SRF "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
	
	if (SRF_PRINT AND SRF_VERISON)
		message(STATUS "${SRF_NAME} SVN revision (from arguments): ${SRF_VERSION}")
	endif ()
	
	if (NOT ${SRF_FILE} MATCHES "^/")
		set(SRF_FILE "${CMAKE_SOURCE_DIR}/${SRF_FILE}")
	endif ()
	if (NOT ${SRF_DIR} MATCHES "^/")
		set(SRF_DIR "${CMAKE_SOURCE_DIR}/${SRF_DIR}")
#		set(SRF_DIR "${CMAKE_CURRENT_BINARY_DIR}/${SRF_DIR}")
	endif ()
	
	file(MAKE_DIRECTORY ${SRF_DIR})
	
	if (SRF_READ AND EXISTS ${SRF_FILE})
		file(READ ${SRF_FILE} SRF_VERSION)
		if (SRF_VERSION)
			string(STRIP ${SRF_VERSION} SRF_VERSION)
			if (SRF_PRINT)
				message(STATUS "${SRF_NAME} SVN revision (from file): ${SRF_VERSION}")
			endif ()
		endif ()
	endif ()
	
	if (SRF_INFO)
		execute_process(COMMAND svn info WORKING_DIRECTORY ${SRF_DIR} OUTPUT_VARIABLE SRF_VERSION)
		string(REGEX REPLACE ".*Revision: ([0-9]+).*" "\\1" SRF_VERSION "${SRF_VERSION}")
		if (SRF_PRINT)
			message(STATUS "${SRF_NAME} SVN revision (from repo): ${SRF_VERSION}")
		endif ()
	endif ()
	
	if (SRF_WRITE)
		file(WRITE ${SRF_FILE} ${SRF_VERSION})
	endif ()
	
	if (SRF_OUTPUT_VARIABLE)
		set(${SRF_OUTPUT_VARIABLE} ${SRF_VERSION} PARENT_SCOPE)
	endif ()
	
	
	if (SRF_CHECKOUT_CMD)
		set(cmake_gen "${PROJECT_BINARY_DIR}/${SRF_NAME}.checkout.cmake")
		
		file(WRITE ${cmake_gen} "set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH})\n")
		file(APPEND ${cmake_gen} "include(SvnRevisionFile)\n")
		file(APPEND ${cmake_gen} "svn_revision_file(DIR ${SRF_DIR} FILE ${SRF_FILE} OUTPUT_VARIABLE data_version READ)\n")
		file(APPEND ${cmake_gen} "if (NOT data_version)\n")
		file(APPEND ${cmake_gen} "\tmessage(STATUS \"No version found, setting to HEAD\")\n")
		file(APPEND ${cmake_gen} "\tset(data_version HEAD)\n")
		file(APPEND ${cmake_gen} "endif ()\n")
		file(APPEND ${cmake_gen} "execute_process(COMMAND svn checkout -r \${data_version} ${SRF_URL} ${SRF_DIR})\n")
	
		add_custom_target(${SRF_CHECKOUT_CMD} COMMAND "${CMAKE_COMMAND}" ARGS -P ${cmake_gen} WORKING_DIRECTORY ${CMAKE_WORKING_DIRECTORY})
	endif ()
	
	if (SRF_COMMIT_CMD)
		set(cmake_gen "${PROJECT_BINARY_DIR}/${SRF_NAME}.commit.cmake")

		file(WRITE ${cmake_gen} "set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH})\n")
		file(APPEND ${cmake_gen} "include(SvnRevisionFile)\n")
		file(APPEND ${cmake_gen} "svn_revision_file(DIR ${SRF_DIR} FILE ${SRF_FILE} INFO WRITE PRINT)\n")
	
		add_custom_target(${SRF_COMMIT_CMD} COMMAND "${CMAKE_COMMAND}" ARGS -P ${cmake_gen} WORKING_DIRECTORY ${CMAKE_WORKING_DIRECTORY})
	endif ()
endfunction(svn_revision_file)


