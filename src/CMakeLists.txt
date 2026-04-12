# EXECUTABLE PROJECT - SorterHunter

# === Create Executable ===
add_executable(SorterHunter   "types.h" "utils.h" "prefix_processor.h"  "utils.cpp" "prefix_processor.cpp" "print.h" "ConvexHull.h" "ConvexHull.cpp" "Config.h" "Config.cpp" "SorterHunter.h" "SorterHunter.cpp" "NetworkMutator.h" "NetworkMutator.cpp" "GlobalRandom.h"  "main.cpp" "version.h" "Network.h" "Network.cpp" "PrefixGenerator.h" "PrefixGenerator.cpp")

# === Feature Enables ===
if (SH_MSVC_STATIC_RUNTIME)
    set_property(TARGET SorterHunter PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif ()
target_compile_features(SorterHunter PRIVATE cxx_std_23)
enable_ipo(SorterHunter)
set_arch(SorterHunter)