project(Test)

add_executable (Test net/net.cpp)
target_link_libraries(Test PRIVATE net)

target_include_directories(Test PRIVATE
	"../"
	"../public"
	"../thirdparty"
	"../thirdparty/doctest"
)