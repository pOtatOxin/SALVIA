
set(SALVIA_BOOST_DIR CACHE PATH "Specify a path to boost.")

if(SALVIA_BOOST_DIR)
	set( BOOST_ROOT ${SALVIA_BOOST_DIR} )
else(SALVIA_BOOST_DIR)
	MESSAGE( FATAL_ERROR "Please specify a path with 'SALVIA_BOOST_DIR' or run build_all.py." )
endif(SALVIA_BOOST_DIR)

FIND_PACKAGE( Boost 1.53.0 )

if( Boost_FOUND )
	set( SALVIA_BOOST_INCLUDE_DIR ${SALVIA_BOOST_DIR} )
else ( Boost_FOUND )	
	MESSAGE( FATAL_ERROR "Cannot find boost 1.53 or later. Please specify a path with 'SALVIA_BOOST_DIR' or run './build_all.py'." )
endif()

if( NOT EXISTS ${SALVIA_BOOST_LIB_DIR} )
	MESSAGE( FATAL_ERROR "Cannot find libraries in ${SALVIA_BOOST_LIB_DIR}. Please compile libraries and copy lib files into directory or run './build_all.py'.")
endif()

set( SALVIA_BOOST_VERSION_STRING "${Boost_MAJOR_VERSION}_${Boost_MINOR_VERSION}" )

# From short name to full path name.
macro( boost_lib_fullname FULL_NAME SHORT_NAME )
	if( SALVIA_BUILD_TYPE_LOWERCASE STREQUAL "debug" )
		set ( SALVIA_BOOST_LIBS_POSTFIX "-d" )
	endif()
	set( ${FULL_NAME} "boost_${SHORT_NAME}-mgw${GCC_VERSION_STR_MAJOR_MINOR}-mt${SALVIA_BOOST_LIBS_POSTFIX}-${SALVIA_BOOST_VERSION_STRING}" )
endmacro( boost_lib_fullname )

macro( add_boost_lib SHORT_NAME )
	set( FULL_NAME "" )
	boost_lib_fullname( FULL_NAME ${SHORT_NAME} )
	set( SALVIA_BOOST_LIBS ${SALVIA_BOOST_LIBS} ${FULL_NAME} )
endmacro( add_boost_lib )

macro( config_boost_libs )
	if(MSVC)
		add_definitions( -DBOOST_ALL_DYN_LINK )
		set( SALVIA_BOOST_LIBS "" )
	else(MSVC)
		add_boost_lib( wave )
		add_boost_lib( unit_test_framework )
		add_boost_lib( program_options )
		add_boost_lib( thread )
		add_boost_lib( date_time )
		add_boost_lib( chrono )
		add_boost_lib( filesystem )
		add_boost_lib( atomic )
		add_boost_lib( system )
	endif(MSVC)
endmacro( config_boost_libs )

config_boost_libs()