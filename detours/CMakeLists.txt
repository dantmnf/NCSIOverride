project(Detours CXX)

add_library(detours STATIC
    detours/src/detours.cpp
    detours/src/modules.cpp
    detours/src/disasm.cpp
    detours/src/image.cpp
    detours/src/creatwth.cpp
    detours/src/disolx86.cpp
    detours/src/disolx64.cpp
    detours/src/disolia64.cpp
    detours/src/disolarm.cpp
    detours/src/disolarm64.cpp
)

target_include_directories(detours INTERFACE ${PROJECT_SOURCE_DIR}/detours/src)
