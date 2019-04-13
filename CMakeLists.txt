PROJECT(OgreOggSound)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

INCLUDE (CheckIncludeFileCXX)

# define header and source files for the library
SET (HEADER_FILES
	include/LocklessQueue.h
    include/OgreOggISound.h
    include/OgreOggListener.h
    include/OgreOggSoundCallback.h
    include/OgreOggSoundFactory.h
    include/OgreOggSound.h
    include/OgreOggSoundManager.h
    include/OgreOggSoundPlugin.h
    include/OgreOggSoundPrereqs.h
    include/OgreOggSoundRecord.h
    include/OgreOggStaticSound.h
    include/OgreOggStaticWavSound.h
    include/OgreOggStreamBufferSound.h
    include/OgreOggStreamSound.h
    include/OgreOggStreamWavSound.h
)

SET (SOURCE_FILES
	src/OgreOggISound.cpp
    src/OgreOggListener.cpp
    src/OgreOggSoundFactory.cpp
    src/OgreOggSoundManager.cpp
    src/OgreOggSoundPlugin.cpp
    src/OgreOggSoundPluginDllStart.cpp
    src/OgreOggSoundRecord.cpp
    src/OgreOggStaticSound.cpp
    src/OgreOggStaticWavSound.cpp
    src/OgreOggStreamBufferSound.cpp
    src/OgreOggStreamSound.cpp
    src/OgreOggStreamWavSound.cpp
)
  
# Option.
SET(OGGSOUND_THREADED NO CACHE BOOL "Enable multi-threaded streamed sounds")
SET(USE_POCO NO CACHE BOOL "Use POCO Threads?")

IF(CMAKE_BUILD_TYPE STREQUAL Debug)

    # Enable all compiler warnings and debug support.
    ADD_DEFINITIONS(-D_DEBUG -g -Wall)
	SET(CMAKE_DEBUG_POSTFIX "_d")
	
ENDIF(CMAKE_BUILD_TYPE STREQUAL Debug)

IF(OGGSOUND_THREADED)
	IF(USE_POCO)
		CHECK_INCLUDE_FILE_CXX(Poco/ScopedLock.h HAVE_POCO_H)
		IF(HAVE_POCO_H)
			ADD_DEFINITIONS(-DOGGSOUND_THREADED=1)
			ADD_DEFINITIONS(-DPOCO_THREAD=1)
		ENDIF()
	ELSE()
		SET(Boost_ADDITIONAL_VERSIONS "1.47" "1.44" "1.43" "1.42")
		FIND_PACKAGE(Boost 1.42.0 COMPONENTS thread)
		IF (Boost_FOUND)
			INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
			LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
			LINK_LIBRARIES(${Boost_LIBRARIES})
			ADD_DEFINITIONS(-DOGGSOUND_THREADED=1)
		ENDIF()
	ENDIF()	 
ELSE()
    ADD_DEFINITIONS(-DOGGSOUND_THREADED=0)
ENDIF()

FIND_PACKAGE(OGRE 1.10 REQUIRED)

FIND_PACKAGE(PkgConfig QUIET)

PKG_CHECK_MODULES(PC_OPENAL QUIET openal)
FIND_PATH(OPENAL_INCLUDE_DIRS NAMES al.h HINTS ${PC_OPENAL_INCLUDE_DIRS} PATH_SUFFIXES openal)
FIND_LIBRARY(OPENAL_LIBRARIES NAMES openal HINTS ${PC_OPENAL_LIBRARY_DIRS})

PKG_CHECK_MODULES(PC_OGG QUIET ogg)
FIND_PATH(OGG_INCLUDE_DIRS NAMES ogg/ogg.h HINTS ${PC_OGG_INCLUDE_DIRS} PATH_SUFFIXES ogg)
FIND_LIBRARY(OGG_LIBRARIES NAMES ogg HINTS ${PC_OGG_LIBRARY_DIRS})

PKG_CHECK_MODULES(PC_VORBISFILE QUIET vorbisfile)
FIND_PATH(VORBIS_INCLUDE_DIRS NAMES vorbis/vorbisfile.h HINTS ${PC_VORBISFILE_INCLUDE_DIRS} PATH_SUFFIXES vorbisfile)
FIND_LIBRARY(VORBISFILE_LIBRARIES NAMES vorbisfile HINTS ${PC_VORBISFILE_LIBRARY_DIRS})

CHECK_INCLUDE_FILE_CXX(efx.h HAVE_EFX)

INCLUDE_DIRECTORIES(include
                    ${OGRE_INCLUDE_DIRS}
                    ${OPENAL_INCLUDE_DIRS}
                    ${OGG_INCLUDE_DIRS}
                    ${VORBISFILE_INCLUDE_DIRS})

LINK_DIRECTORIES(${OGRE_LIBRARY_DIRS}
                 ${OPENAL_LIBRARY_DIRS}
                 ${OGG_INCLUDE_DIRS}
                 ${VORBISFILE_INCLUDE_DIRS})

LINK_LIBRARIES(${OGRE_LIBRARIES}
               ${OPENAL_LIBRARIES}
               ${OGG_LIBRARIES}
               ${VORBISFILE_LIBRARIES})

ADD_LIBRARY(Plugin_OggSound SHARED ${SOURCE_FILES} ${HEADER_FILES})
# disable "lib" prefix
set_target_properties(Plugin_OggSound PROPERTIES PREFIX "")

set(CMAKE_INSTALL_PREFIX ${OGRE_PREFIX_DIR} CACHE PATH "plugin must end up in OGRE plugin path" FORCE)
INSTALL(FILES ${HEADER_FILES} DESTINATION include/OGRE/Plugins/OggSound)
INSTALL(TARGETS Plugin_OggSound LIBRARY DESTINATION lib/OGRE/ ARCHIVE DESTINATION lib/OGRE/)
set_property(TARGET Plugin_OggSound PROPERTY INSTALL_RPATH ${OGRE_LIBRARY_DIRS})

# doxygen stuff
find_package(Doxygen)
if (DOXYGEN_FOUND)
  # prepare doxygen configuration file
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

  add_custom_target( docs
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating documentation with Doxygen."
    VERBATIM
  )

  # cleanup $build/api-doc on "make clean"
  set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES docs)
endif (DOXYGEN_FOUND)
