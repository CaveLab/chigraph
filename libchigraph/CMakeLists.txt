# chigraph CMakeLists.txt

# Create the library
set(CHI_PUBLIC_FILES
	include/chi/ChiModule.hpp
	include/chi/Context.hpp
	include/chi/GraphFunction.hpp
	include/chi/GraphModule.hpp
	include/chi/LangModule.hpp
	include/chi/NodeInstance.hpp
	include/chi/NodeType.hpp
	include/chi/Result.hpp
	include/chi/NameMangler.hpp
	include/chi/FunctionValidator.hpp
	include/chi/FunctionCompiler.hpp
	include/chi/JsonDeserializer.hpp
	include/chi/JsonSerializer.hpp
	include/chi/GraphStruct.hpp
)
set(CHI_PRIVATE_FILES
	src/Context.cpp
	src/NodeInstance.cpp
	src/GraphFunction.cpp
	src/LangModule.cpp
	src/GraphModule.cpp
	src/ChiModule.cpp
	src/Result.cpp
	src/NameMangler.cpp
	src/FunctionValidator.cpp
	src/FunctionCompiler.cpp
	src/JsonDeserializer.cpp
	src/JsonSerializer.cpp
	src/GraphStruct.cpp
	src/NodeType.cpp
	src/DataType.cpp
)
add_library(chigraph STATIC ${CHI_PUBLIC_FILES} ${CHI_PRIVATE_FILES})

# Make sure we have a good enough compiler

# check that if we're using MSVC it's 2015+
if (MSVC AND MSVC_VERSION LESS 1900)
	message(FATAL_ERROR "Cannot use chigraph with MSVC older than 2015, please upgrade to 2015 or newer")
endif()

# Make sure our compiler supports the things I (and dependencies) use
if(${CMAKE_VERSION_MAJOR}.${CMAKE_VERSION_MINOR} VERSION_LESS 3.1)
	CXX11_CHECK_FEATURE("rvalue_references"  2118 HAS_CXX11_RVALUE_REFERENCES)
	CXX11_CHECK_FEATURE("variadic_templates" 2555 HAS_CXX11_VARIADIC_TEMPLATES)
	CXX11_CHECK_FEATURE("constexpr"          2235 HAS_CXX11_CONSTEXPR)
	
	# and make sure we get the compile option to enable it if the compiler needs it
	if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR 
		CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" OR
		CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		target_compile_options(chigraph PUBLIC -std=c++14)
	endif()
else()
	# if we have an new enough cmake, then we can just use this call that does all that for us
	target_compile_features(chigraph 
	PUBLIC 
		cxx_aggregate_default_initializers
		cxx_alias_templates
		cxx_auto_type
		cxx_defaulted_functions
		cxx_deleted_functions
		cxx_generic_lambdas
		cxx_inline_namespaces
		cxx_lambdas
		cxx_lambda_init_captures
		cxx_noexcept
		cxx_nullptr
		cxx_override
		cxx_range_for
		cxx_raw_string_literals
		cxx_right_angle_brackets
		cxx_rvalue_references
		cxx_strong_enums
		cxx_variadic_templates
		cxx_variable_templates
	)
endif()


# get llvm libraries
set(LLVM_COMPONENTS
	executionengine
	profiledata
	instrumentation
	irreader
	option
	bitwriter
	native
	interpreter
	lto
	objcarcopts
	nativecodegen
	linker
	mcjit
	scalaropts
	transformutils
	codegen
	selectiondag
)
if (LLVM_VERSION VERSION_GREATER 3.7.0 OR LLVM_VERSION VERSION_EQUAL 3.7.0)
	list(APPEND LLVM_COMPONENTS passes)
endif()
if (LLVM_VERSION VERSION_GREATER 3.9.0 OR LLVM_VERSION VERSION_EQUAL 3.9.0)
	list(APPEND LLVM_COMPONENTS coverage)
endif()
if (LLVM_VERSION VERSION_GREATER 4.0.0 OR LLVM_VERSION VERSION_EQUAL 4.0.0)
	list(APPEND LLVM_COMPONENTS coroutines)
endif()


execute_process(COMMAND ${LLVM_CONFIG} --libs ${LLVM_COMPONENTS} OUTPUT_VARIABLE LLVM_LIBRARIES OUTPUT_STRIP_TRAILING_WHITESPACE)
string(REPLACE " " ";" LLVM_LINK_LIST "${LLVM_LIBRARIES}")
message(STATUS "LLVM link libraries: ${LLVM_LINK_LIST}")

# get system libraries to link to
execute_process(COMMAND ${LLVM_CONFIG} --system-libs OUTPUT_VARIABLE LLVM_SYSTEM_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)
string(REPLACE " " ";" LLVM_SYSTEM_LINK_LIST "${LLVM_SYSTEM_LIBS}")
message(STATUS "LLVM system libraries: ${LLVM_SYSTEM_LINK_LIST}")

# get preprocessor flags
execute_process(COMMAND ${LLVM_CONFIG} --cppflags OUTPUT_VARIABLE LLVM_CXX_FLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
string(REPLACE " " ";" LLVM_CXX_FLAGS_LIST "${LLVM_CXX_FLAGS}")
message(STATUS "LLVM cxx flags: ${LLVM_CXX_FLAGS_LIST}")

# get ld flags
execute_process(COMMAND ${LLVM_CONFIG} --ldflags OUTPUT_VARIABLE LLVM_LD_FLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
string(REPLACE " " ";" LLVM_LD_FLAGS_LIST "${LLVM_LD_FLAGS}")
message(STATUS "LLVM ld flags: ${LLVM_LD_FLAGS_LIST}")

target_link_libraries(chigraph
PUBLIC
	${Boost_SYSTEM_LIBRARY}
	${Boost_FILESYSTEM_LIBRARY}
	${LIBGIT2_LIBRARIES}
	${LLVM_LD_FLAGS_LIST}
	${LLVM_LINK_LIST}
	${LLVM_SYSTEM_LINK_LIST}
	ffi # llvm often forgets to put this in --system-libs, so just link it anyways just in case
PRIVATE
	libchigraphexecstream
)

if(WIN32)

	find_library(VERSION_LIBRARY version)
	if (NOT VERSION_LIBRARY)
		message(FATAL_ERROR "Could not find version library")
	endif()
	message(STATUS "Linking to version library: ${VERSION_LIBRARY}")
	target_link_libraries(chigraph
	PUBLIC
		${VERSION_LIBRARY}
	)
endif()

# get include directories
execute_process(COMMAND ${LLVM_CONFIG} --includedir OUTPUT_VARIABLE LLVM_INCLUDE_DIR  OUTPUT_STRIP_TRAILING_WHITESPACE)

target_include_directories(chigraph
PUBLIC
	include/
	${LIBGIT2_INCLUDE_DIR}
	${LLVM_INCLUDE_DIR}
)

target_compile_options(chigraph
PUBLIC 
	${LLVM_CXX_FLAGS_LIST}
	${LIBGIT2_DEFINITIONS}
)

# configure the config file
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/ChigraphConfig.cmake.in ${CMAKE_BINARY_DIR}/ChigraphConfig.cmake @ONLY)

# make sure chi-ctollvm is built first
add_dependencies(chigraph chi-ctollvm)

install(FILES ${CMAKE_BINARY_DIR}/ChigraphConfig.cmake DESTINATION lib/cmake/chigraph)

install(TARGETS chigraph
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)

install(DIRECTORY include/chi DESTINATION include)

