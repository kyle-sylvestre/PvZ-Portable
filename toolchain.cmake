set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Compilers
set(PFIX "/opt/homebrew/Cellar/mingw-w64/13.0.0/bin")
set(CMAKE_C_COMPILER ${PFIX}/x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER ${PFIX}/x86_64-w64-mingw32-g++)

# Optional: resource compiler
set(CMAKE_RC_COMPILER ${PFIX}/x86_64-w64-mingw32-windres)

# Where to search for libs/headers
#set(CMAKE_FIND_ROOT_PATH "/opt/homebrew/Cellar/mingw-w64/13.0.0")
set(CMAKE_FIND_ROOT_PATH "/Users/ksylvestre/dev/PvZ-Portable/winbuild/mingw")

# Adjust search behavior
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Avoid macOS flags leaking in
set(CMAKE_C_FLAGS "")
set(CMAKE_CXX_FLAGS "")
