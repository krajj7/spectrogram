# - Find SAMPLERATE
# Find the native SAMPLERATE includes and libraries
#
#  SAMPLERATE_INCLUDE_DIR - where to find SAMPLERATE.h, etc.
#  SAMPLERATE_LIBRARIES   - List of libraries when using libSAMPLERATE.
#  SAMPLERATE_FOUND       - True if libSAMPLERATE found.

if(SAMPLERATE_INCLUDE_DIR)
    # Already in cache, be silent
    set(SAMPLERATE_FIND_QUIETLY TRUE)
endif(SAMPLERATE_INCLUDE_DIR)

find_path(SAMPLERATE_INCLUDE_DIR samplerate.h)

find_library(SAMPLERATE_LIBRARY NAMES samplerate)

# Handle the QUIETLY and REQUIRED arguments and set SAMPLERATE_FOUND to TRUE if
# all listed variables are TRUE.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SAMPLERATE DEFAULT_MSG
    SAMPLERATE_INCLUDE_DIR SAMPLERATE_LIBRARY)

if(SAMPLERATE_FOUND)
  set(SAMPLERATE_LIBRARIES ${SAMPLERATE_LIBRARY})
else(SAMPLERATE_FOUND)
  set(SAMPLERATE_LIBRARIES)
endif(SAMPLERATE_FOUND)

#mark_as_advanced(SAMPLERATE_INCLUDE_DIR SAMPLERATE_LIBRARY)
