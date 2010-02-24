/**
* @file OgreOggSoundPlugin.h
* @author  Ian Stangoe
* @version 1.15
*
* @section LICENSE
* 
* This source file is part of OgreOggSound, an OpenAL wrapper library for   
* use with the Ogre Rendering Engine.										 
*                                                                           
* Copyright 2009 Ian Stangoe 
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
* Impements the plugin interface for OGRE
*/

#ifndef __OGREOGGSOUNDPlugin_H__
#define __OGREOGGSOUNDPlugin_H__

#include <OgrePlugin.h>

#include "OgreOggSound.h"
#include "OgreOggSoundFactory.h"


namespace OgreOggSound
{

	//! Plugin instance for the MovableText 
	class OgreOggSoundPlugin : public Ogre::Plugin
	{

	public:

		OgreOggSoundPlugin();

		/// @copydoc Plugin::getName
		const Ogre::String& getName() const;

		/// @copydoc Plugin::install
		void install();

		/// @copydoc Plugin::initialise
		void initialise();

		/// @copydoc Plugin::shutdown
		void shutdown();

		/// @copydoc Plugin::uninstall
		void uninstall();

	protected:

		OgreOggSoundFactory* mOgreOggSoundFactory;
		OgreOggSoundManager* mOgreOggSoundManager;


	};
}

#endif
