# EXECUTABLE PROJECT - SorterHunter

# === Create Executable ===
add_executable(SorterHunter   "types.h" "utils.h" "prefix_processor.h"  "utils.cpp" "prefix_processor.cpp" "print.h" "ConvexHull.h" "ConvexHull.cpp" "Config.h" "Config.cpp" "SorterHunter.h" "SorterHunter.cpp" "NetworkMutator.h" "NetworkMutator.cpp" "GlobalRandom.h"  "main.cpp" "version.h" "Network.h" "Network.cpp" "PrefixGenerator.h" "PrefixGenerator.cpp" "Timer.h"   "GlobalRandom.cpp" "rendering.h" "rendering.cpp")

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND UNIX)
    target_compile_options(SorterHunter PRIVATE -stdlib=libc++)
    target_link_options(SorterHunter PRIVATE -stdlib=libc++)
endif()

find_package(Threads REQUIRED)
find_package(raylib REQUIRED)
target_link_libraries(SorterHunter PRIVATE Threads::Threads raylib winmm)

# === Feature Enables ===
if (SH_MSVC_STATIC_RUNTIME)
    set_property(TARGET SorterHunter PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif ()
target_compile_features(SorterHunter PRIVATE cxx_std_23)
enable_ipo(SorterHunter)
set_arch(SorterHunter)
if (UNIX)
    set_target_properties(SorterHunter PROPERTIES
		BUILD_RPATH "/apps/developers/compilers/llvm/20.1.8/1/default/lib/x86_64-unknown-linux-gnu"
	)
endif()