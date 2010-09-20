set ( SOFTART_HOME_DIR "${CMAKE_HOME_DIRECTORY}/../softart/" )
set ( SASL_HOME_DIR	"${CMAKE_HOME_DIRECTORY}/../" )

################ configurate 3rd party library paths ###############
set( 3rd_party_path "${SOFTART_HOME_DIR}/../3rd_party/" )
	
################ set llvm path ################
set( 3rd_party_llvm_path "${3rd_party_path}/llvm/" )
set( 3rd_party_llvm_include_path
	"${3rd_party_llvm_path}/include/" "${3rd_party_llvm_path}/config/${SOFTART_ENV_WITHOUT_BUILD_TYPE}/"
	)
set( 3rd_party_llvm_lib_path "${3rd_party_llvm_path}/lib/${SOFTART_ENVIRONMENT_NAME}" )