/*---------------------------------------------------------------------------*\
** This source file is part of OgreOggSound, an OpenAL wrapper library for   **
** use with the Ogre Rendering Engine.										 **
**                                                                           **
** Copyright 2008 Ian Stangoe & Eric Boissard								 **
**                                                                           **
** OgreOggSound is free software: you can redistribute it and/or modify		 ** 
** it under the terms of the GNU Lesser General Public License as published	 **
** by the Free Software Foundation, either version 3 of the License, or		 **
** (at your option) any later version.										 **
**																			 **
** OgreOggSound is distributed in the hope that it will be useful,			 **
** but WITHOUT ANY WARRANTY; without even the implied warranty of			 **
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the			 **
** GNU Lesser General Public License for more details.						 **
**																			 **
** You should have received a copy of the GNU Lesser General Public License	 **
** along with OgreOggSound.  If not, see <http://www.gnu.org/licenses/>.	 **
\*---------------------------------------------------------------------------*/

#pragma once

/**
 * DLL linkage
 */
#ifdef OGGSOUND_EXPORT
	#define _OGGSOUND_EXPORT __declspec(dllexport)
#else
	#define _OGGSOUND_EXPORT __declspec(dllimport)
#endif

/**
 * Specifies whether to use threads for streaming
 * 0 - No multithreading 
 * 1 - BOOST multithreaded 
 */
#ifndef OGGSOUND_THREADED
#	define OGGSOUND_THREADED 1
#endif


