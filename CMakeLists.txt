CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

#SET(CMAKE_VERBOSE_MAKEFILE on)

PROJECT(spectrogram)
INCLUDE_DIRECTORIES( ${CMAKE_BINARY_DIR} )

### extra find modules for cmake

LIST(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake-modules)

### files

SET(spectrogram_SOURCES
    main.cpp
    mainwindow.cpp
    spectrogram.cpp
    soundfile.cpp
    fft.cpp
)
SET(spectrogram_MOC_HEADERS 
    mainwindow.hpp
    spectrogram.hpp
)
SET(spectrogram_UIS 
    mainwindow.ui
)
SET(spectrogram_RCS 
    # none (yet?)
)

### compiler options

# enable warnings
IF (CMAKE_COMPILER_IS_GNUCXX)
  ADD_DEFINITIONS("-Wall -Wextra")
ENDIF (CMAKE_COMPILER_IS_GNUCXX)

### find and use Qt4

# SET(QT_USE_QTXML TRUE) # XXX moduly takhle
FIND_PACKAGE(Qt4 REQUIRED)
## Qt macros
INCLUDE(${QT_USE_FILE})
QT4_ADD_RESOURCES( spectrogram_RC_SOURCES ${spectrogram_RCS} )
# this will run uic on .ui files:
QT4_WRAP_UI( spectrogram_UI_HEADERS ${spectrogram_UIS} )
# this will run moc:
QT4_WRAP_CPP( spectrogram_MOC_SOURCES ${spectrogram_MOC_HEADERS} )
INCLUDE_DIRECTORIES(${QT_INCLUDE_DIR})

### find libsndfile

FIND_PACKAGE(Sndfile REQUIRED)
INCLUDE_DIRECTORIES(${SNDFILE_INCLUDE_DIR})

### find libsamplerate

FIND_PACKAGE(LibSamplerate REQUIRED)
INCLUDE_DIRECTORIES(${SAMPLERATE_INCLUDE_DIR})

### find fftw3

SET(FFTW3_FIND_QUIETLY TRUE)
FIND_PACKAGE(FFTW3 REQUIRED)
INCLUDE_DIRECTORIES(${FFTW3_INCLUDES})

### find libmad

FIND_PACKAGE(Mad REQUIRED)
INCLUDE_DIRECTORIES(${MAD_INCLUDE_DIRS})

### included libs
#ADD_SUBDIRECTORY(fftw++)

#### find boost
#FIND_PACKAGE(Boost COMPONENTS filesystem REQUIRED)
#INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})

### debug option

OPTION(spectrogram_DEBUG "Build the project with debugging code" ON) # XXX ->OFF
IF(spectrogram_DEBUG)
  SET(CMAKE_BUILD_TYPE Debug)
  SET_SOURCE_FILES_PROPERTIES(${spectrogram_SOURCES} COMPILE_FLAGS -DDEBUG)
ELSE(spectrogram_DEBUG)
  SET(CMAKE_BUILD_TYPE Release)
  IF(WIN32)
    # prevents the console window from opening on windows
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mwindows")
  ENDIF(WIN32)
ENDIF(spectrogram_DEBUG)

### finish

ADD_EXECUTABLE(spectrogram ${spectrogram_SOURCES} ${spectrogram_MOC_SOURCES} ${spectrogram_UI_HEADERS} ${spectrogram_RC_SOURCES})

TARGET_LINK_LIBRARIES(spectrogram ${QT_LIBRARIES} ${SNDFILE_LIBRARIES} ${FFTW3_LIBRARIES} ${SAMPLERATE_LIBRARIES} ${MAD_LIBRARIES})
