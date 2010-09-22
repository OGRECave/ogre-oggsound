/**
* @file OgreOggSoundPluginDllStart.cpp
* @author  Ian Stangoe
* @version 1.18
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
*/

#include <OgreRoot.h>

#include "OgreOggSound.h"
#include "OgreOggSoundPlugin.h"

using namespace Ogre;
using namespace OgreOggSound;

OgreOggSoundPlugin* mOgreOggSoundPlugin;

//----------------------------------------------------------------------------------
// register SecureArchive with Archive Manager
//----------------------------------------------------------------------------------
extern "C" void _OGGSOUND_EXPORT dllStartPlugin( void )
{
	// Create new plugin
	mOgreOggSoundPlugin = OGRE_NEW_T(OgreOggSoundPlugin, Ogre::MEMCATEGORY_GENERAL)();

	// Register
	Root::getSingleton().installPlugin(mOgreOggSoundPlugin);

}
extern "C" void _OGGSOUND_EXPORT dllStopPlugin( void )
{
	Root::getSingleton().uninstallPlugin(mOgreOggSoundPlugin);

	OGRE_DELETE_T(mOgreOggSoundPlugin, OgreOggSoundPlugin, Ogre::MEMCATEGORY_GENERAL);
	mOgreOggSoundPlugin = 0;
}
