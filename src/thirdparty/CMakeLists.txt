set(BUILD_SHARED_LIBS OFF)

add_subdirectory(KeyValues)
add_subdirectory(json)

set(WOLFSSL_OPENSSLEXTRA "yes")
set(WOLFSSL_EXAMPLES "no")
set(WOLFSSL_ARC4 "yes")
add_subdirectory(wolfssl)
target_compile_definitions(wolfssl PRIVATE WOLFSSL_ALLOW_RC4=1)

if (SERVER_DBSQLITE)
	set(SQLITECPP_USE_STATIC_RUNTIME OFF CACHE BOOL "Use MSVC static runtime (default for internal googletest).")
	# Disable stupid linting error
	set(SQLITECPP_RUN_CPPLINT OFF CACHE BOOL "Run cpplint.py tool for Google C++ StyleGuide." FORCE)
	add_subdirectory(SQLiteCpp)
endif()

add_subdirectory(zip)