#+-----------------------------------------------------------------------------+
#|   Copyright (C) 2007                                                        |
#|   Lars B"ahren (bahren@astron.nl)                                           |
#|                                                                             |
#|   This program is free software; you can redistribute it and/or modify      |
#|   it under the terms of the GNU General Public License as published by      |
#|   the Free Software Foundation; either version 2 of the License, or         |
#|   (at your option) any later version.                                       |
#|                                                                             |
#|   This program is distributed in the hope that it will be useful,           |
#|   but WITHOUT ANY WARRANTY; without even the implied warranty of            |
#|   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             |
#|   GNU General Public License for more details.                              |
#|                                                                             |
#|   You should have received a copy of the GNU General Public License         |
#|   along with this program; if not, write to the                             |
#|   Free Software Foundation, Inc.,                                           |
#|   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.                 |
#+-----------------------------------------------------------------------------+
# modified version.

# - Check for the presence of FFTW3
#
# The following variables are set when FFTW3 is found:
#  HAVE_FFTW3       = Set to true, if all components of FFTW3 have been found.
#  FFTW3_INCLUDES   = Include path for the header files of FFTW3
#  FFTW3_LIBRARIES  = Link these to use FFTW3

## -----------------------------------------------------------------------------
## Search locations

#include (CMakeSettings)

## -----------------------------------------------------------------------------
## Check for the header files

FIND_PATH (FFTW3_INCLUDES fftw3.h
  PATHS
  ${include_locations}
  /opt/aips++/local/include
  )

## -----------------------------------------------------------------------------
## Check for the library

FIND_LIBRARY (FFTW3_LIBRARIES fftw3f fftwf
  PATHS
  ${lib_locations}
  /opt/aips++/local/lib
  )

## -----------------------------------------------------------------------------
## Actions taken when all components have been found

IF (FFTW3_INCLUDES AND FFTW3_LIBRARIES)
  SET (HAVE_FFTW3 TRUE)
ELSE (FFTW3_INCLUDES AND FFTW3_LIBRARIES)
  SET (HAVE_FFTW3 FALSE)
  IF (NOT FFTW3_FIND_QUIETLY)
    IF (NOT FFTW3_INCLUDES)
      MESSAGE (STATUS "Unable to find FFTW3 header files!")
    ENDIF (NOT FFTW3_INCLUDES)
    IF (NOT FFTW3_LIBRARIES)
      MESSAGE (STATUS "Unable to find FFTW3 library files!")
    ENDIF (NOT FFTW3_LIBRARIES)
  ENDIF (NOT FFTW3_FIND_QUIETLY)
ENDIF (FFTW3_INCLUDES AND FFTW3_LIBRARIES)

IF (HAVE_FFTW3)
  IF (NOT FFTW3_FIND_QUIETLY)
    MESSAGE (STATUS "Found components for FFTW3")
    MESSAGE (STATUS "FFTW3_INCLUDES  = ${FFTW3_INCLUDES}")
    MESSAGE (STATUS "FFTW3_LIBRARIES = ${FFTW3_LIBRARIES}")
  ENDIF (NOT FFTW3_FIND_QUIETLY)
ELSE (HAVE_FFTW3)
  IF (FFTW3_FIND_REQUIRED)
    MESSAGE (FATAL_ERROR "Could not find FFTW3!")
  ENDIF (FFTW3_FIND_REQUIRED)
ENDIF (HAVE_FFTW3)

## ------------------------------------------------------------------------------
## Mark as advanced ...

