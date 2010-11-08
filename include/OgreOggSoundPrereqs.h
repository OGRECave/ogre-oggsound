/**
* @file OgreOggSoundPrereqs.h
* @author  Ian Stangoe
* @version 1.19
*
* @section LICENSE
* 
* This source file is part of OgreOggSound, an OpenAL wrapper library for   
* use with the Ogre Rendering Engine.										 
*                                                                           
* Copyright 2010 Ian Stangoe 
*                                                                           
* OgreOggSound is free software: you can redistribute it and/or modify		  
* it under the terms of the GNU Lesser General Public License as published	 
* by the Free Software Foundation, either version 3 of the License, or		 
* (at your option) any later version.										 
*																			 
* OgreOggSound is distributed in the hope that it will be useful,			 
* but WITHOUT ANY WARRANTY; without even the implied warranty of			 
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the			 
* GNU Lesser General Public License for more details.						 
*																			 
* You should have received a copy of the GNU Lesser General Public License	 
* along with OgreOggSound.  If not, see <http://www.gnu.org/licenses/>.	 
*
* @section DESCRIPTION
* 
* Pre-requisites for building lib
*/

#include <Ogre.h>

#   if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#	pragma once
#	pragma warning( disable : 4244 )

#	include "al.h"
#	include "alc.h"
#	ifdef HAVE_EFX
#		include "efx.h"
#		include "efx-util.h"
#		include "efx-creative.h"
#		include "xram.h"
#	endif
#	if OGRE_COMPILER == OGRE_COMPILER_MSVC
#		ifdef OGGSOUND_EXPORT
#			define _OGGSOUND_EXPORT __declspec(dllexport)
#		else
#			define _OGGSOUND_EXPORT __declspec(dllimport)
#		endif
#	else
#		define _OGGSOUND_EXPORT
#	endif
#elif OGRE_COMPILER == OGRE_COMPILER_GNUC
#   if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
#		include <al.h>
#		include <alc.h>
#   else
#		include <AL/al.h>
#		include <AL/alc.h>
#	endif
#	if defined(OGGSOUND_EXPORT) && OGRE_COMP_VER >= 400
#		define _OGGSOUND_EXPORT __attribute__ ((visibility("default")))
#	else
#		define _OGGSOUND_EXPORT
#	endif
#else // Other Compilers
#	include <OpenAL/al.h>
#	include <OpenAL/alc.h>
#	include "xram.h"
#	define _OGGSOUND_EXPORT
#endif

/**
 * Specifies whether to use threads for streaming
 * 0 - No multithreading
 * 1 - BOOST multithreaded
 */
#ifndef OGGSOUND_THREADED
	#define OGGSOUND_THREADED 1
#endif


