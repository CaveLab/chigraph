# compile + install a c++/c standard library

include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/GetLibStdCXX.cmake)

execute_process(COMMAND ${LLVM_CONFIG} --prefix OUTPUT_VARIABLE CLANG_INSTALL_PREFIX  OUTPUT_STRIP_TRAILING_WHITESPACE)
message(STATUS "clang install prefix: ${CLANG_INSTALL_PREFIX}")

execute_process(COMMAND ${LLVM_CONFIG} --libdir OUTPUT_VARIABLE CLANG_LIB_DIR  OUTPUT_STRIP_TRAILING_WHITESPACE)
message(STATUS "clang libdir: ${CLANG_LIB_DIR}")

add_subdirectory(ctollvm)

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
	include/chi/CModule.hpp
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
	src/CModule.cpp
	src/Result.cpp
	src/NameMangler.cpp
	src/FunctionValidator.cpp
	src/FunctionCompiler.cpp
	src/JsonDeserializer.cpp
	src/JsonSerializer.cpp
	src/GraphStruct.cpp
)


add_library(chigraph STATIC ${CHI_PUBLIC_FILES} ${CHI_PRIVATE_FILES})

target_compile_features(chigraph PUBLIC cxx_constexpr cxx_rvalue_references cxx_variable_templates)

# get include directories
execute_process(COMMAND ${LLVM_CONFIG} --includedir OUTPUT_VARIABLE LLVM_INCLUDE_DIR  OUTPUT_STRIP_TRAILING_WHITESPACE)
message(STATUS "LLVM include directory: ${LLVM_INCLUDE_DIR}")

# get llvm libraries
set(LLVM_COMPONENTS
	executionengine
	native
	interpreter
	nativecodegen
	linker
	mcjit
	codegen
	selectiondag
)
if (LLVM_VERSION VERSION_GREATER 3.9.0 OR LLVM_VERSION VERSION_EQUAL 3.9.0)
	list(APPEND LLVM_COMPONENTS coverage)
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

link_directories(${CLANG_LIB_DIR})

target_link_libraries(chigraph
PUBLIC
	libexecstream
	boost_filesystem
	${LLVM_LD_FLAGS_LIST}
	${LLVM_LINK_LIST}
PRIVATE
	ctollvm
PUBLIC
	${LLVM_SYSTEM_LINK_LIST}
	ffi # llvm often forgets to put this in --system-libs, so just link it anyways just in case
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

target_include_directories(chigraph
PUBLIC
	include/
	${CMAKE_BINARY_DIR}/ # for config.h
	../third_party/GSL/include
	../third_party/libexecstream/include
	${LLVM_INCLUDE_DIR}
)

target_compile_options(chigraph
PUBLIC 
	${LLVM_CXX_FLAGS_LIST}
)


# move the clang lib folder, this is basic includes
add_custom_command(
TARGET chigraph POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_directory
	${CLANG_LIB_DIR}/clang
	${CMAKE_BINARY_DIR}/lib/clang
BYPRODUCTS ${CMAKE_BINARY_DIR}/lib/clang
COMMENT "Copying standard clang includes from ${CLANG_LIB_DIR}/clang to ${CMAKE_BINARY_DIR}/lib/clang"
)

install(TARGETS chigraph 
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)
install(DIRECTORY include/chi DESTINATION include)
install(DIRECTORY ${CMAKE_BINARY_DIR}/lib DESTINATION .)
