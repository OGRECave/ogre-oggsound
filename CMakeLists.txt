
PROJECT(OgreOggSound)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

# Option.
SET(OGGSOUND_THREADED YES CACHE BOOL "Enable multi-threaded streamed sounds")
SET(USE_BOOST YES CACHE BOOL "Use Boost Threads?")

IF(CMAKE_BUILD_TYPE STREQUAL Debug)

    # Enable all compiler warnings and debug support.
    ADD_DEFINITIONS(-D_DEBUG -g -Wall)

ENDIF(CMAKE_BUILD_TYPE STREQUAL Debug)

IF(OGGSOUND_THREADED)
    ADD_DEFINITIONS(-DOGGSOUND_THREADED=1)

    IF(USE_BOOST)
        ADD_DEFINITIONS(-DBOOST_THREAD=1)

    ELSE(USE_BOOST)
        ADD_DEFINITIONS(-DPOCO_THREAD=1)

    ENDIF(USE_BOOST)

ELSE(OGGSOUND_THREADED)
    ADD_DEFINITIONS(-DOGGSOUND_THREADED=0)

ENDIF(OGGSOUND_THREADED)

IF (APPLE)
   FIND_PACKAGE(OPENAL REQUIRED)
   FIND_LIBRARY(OGRE OGRE REQUIRED)
   FIND_LIBRARY(OGG Ogg REQUIRED)
   FIND_LIBRARY(VORBIS Vorbis REQUIRED)

   set(OGG_LIBRARIES ${OGG})
   set(VORBISFILE_LIBRARIES ${VORBIS})
   set(VORBISFILE_INCLUDE_DIRS ${VORBIS_INCLUDE_DIRS})   
   set(OPENAL_INCLUDE_DIRS ${OPENAL_INCLUDE_DIRS} ${OPENAL_INCLUDE_DIR})
   set(OPENAL_LIBRARIES ${OPENAL_LIBRARIES} ${OPENAL_LIBRARY})
ELSE (APPLE) 
    FIND_PACKAGE(PkgConfig REQUIRED)
    PKG_CHECK_MODULES(OGRE REQUIRED OGRE)
    PKG_CHECK_MODULES(OPENAL REQUIRED openal)
    PKG_CHECK_MODULES(OGG REQUIRED ogg)
    PKG_CHECK_MODULES(VORBISFILE REQUIRED vorbisfile)
ENDIF (APPLE)

AUX_SOURCE_DIRECTORY(src OGREOGGSOUND_SRCS)

INCLUDE_DIRECTORIES(include
                    ${OGRE_INCLUDE_DIRS}
                    ${OPENAL_INCLUDE_DIRS}
                    ${OGG_INCLUDE_DIRS}
                    ${VORBISFILE_INCLUDE_DIRS}
                    /usr/include/AL
                    /usr/local/include/AL)

LINK_DIRECTORIES(${OGRE_LIBRARY_DIRS}
                 ${OPENAL_LIBRARY_DIRS}
                 ${OGG_INCLUDE_DIRS}
                 ${VORBISFILE_INCLUDE_DIRS})

LINK_LIBRARIES(${OGRE_LIBRARIES}
               ${OPENAL_LIBRARIES}
               ${OGG_LIBRARIES}
               ${VORBISFILE_LIBRARIES})

ADD_LIBRARY(${PROJECT_NAME} SHARED ${OGREOGGSOUND_SRCS})

INSTALL(FILES include/LocklessQueue.h
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
              include/OgreOggStreamSound.h
              include/OgreOggStreamWavSound.h
        DESTINATION include)

INSTALL(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib)
