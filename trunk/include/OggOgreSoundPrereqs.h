/*---------------------------------------------------------------------------*\
** This source file is part of OggOgreSound, an OpenAL wrapper library for   **
** use with the Ogre Rendering Engine.										 **
**                                                                           **
** Copyright 2008 Ian Stangoe & Eric Boissard								 **
**                                                                           **
** OggOgreSound is free software: you can redistribute it and/or modify		 ** 
** it under the terms of the GNU Lesser General Public License as published	 **
** by the Free Software Foundation, either version 3 of the License, or		 **
** (at your option) any later version.										 **
**																			 **
** OggOgreSound is distributed in the hope that it will be useful,			 **
** but WITHOUT ANY WARRANTY; without even the implied warranty of			 **
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the			 **
** GNU Lesser General Public License for more details.						 **
**																			 **
** You should have received a copy of the GNU Lesser General Public License	 **
** along with OggOgreSound.  If not, see <http://www.gnu.org/licenses/>.	 **
\*---------------------------------------------------------------------------*/

#pragma once

#ifdef OGGSOUND_EXPORT
	#define _OGGSOUND_EXPORT __declspec(dllexport)
#else
	#define _OGGSOUND_EXPORT __declspec(dllimport)
#endif