# EXECUTABLE PROJECT - SorterHunter

# === Create Executable ===
add_executable(SorterHunter   "Types/types.h"  "Prefix/prefix_processor.h"   "Prefix/prefix_processor.cpp" "Utility/print.h" "Types/ConvexHull.h" "Types/ConvexHull.cpp" "Config/Config.h" "Config/Config.cpp" "Genetic/SimpleGeneticHunter.h" "Genetic/SimpleGeneticHunter.cpp" "Genetic/NetworkMutator.h" "Genetic/NetworkMutator.cpp" "Utility/GlobalRandom.h"  "main.cpp" "Utility/version.h" "Types/Network.h" "Types/Network.cpp" "Prefix/PrefixGenerator.h" "Prefix/PrefixGenerator.cpp" "Timer/Timer.h"   "Utility/GlobalRandom.cpp" "UI/rendering.h" "UI/rendering.cpp" "Ants/AntColonyHunter.h" "Ants/AntColonyHunter.cpp" "SorterHunter.h" "SorterHunter.cpp" "Ants/CEGraph.h" "Ants/WeightedSampler.h" "Ants/WeightedSampler.cpp" "Ants/CEGraph.cpp")

target_include_directories(SorterHunter PRIVATE ".")

# === Libraries ===
find_package(Threads REQUIRED)
find_package(raylib REQUIRED)
target_link_libraries(SorterHunter PRIVATE Threads::Threads raylib winmm)

# === Feature Enables ===
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND UNIX)
    target_compile_options(SorterHunter PRIVATE -stdlib=libc++)
    target_link_options(SorterHunter PRIVATE -stdlib=libc++)
endif()
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