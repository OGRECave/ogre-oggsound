/**
* @file OgreOggSoundPlugin.cpp
* @author  Ian Stangoe
* @version 1.18
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
*/

#include "OgreOggSoundPlugin.h"

using namespace Ogre;
using namespace OgreOggSound;

const String sPluginName = "OgreOggSound";

//---------------------------------------------------------------------
OgreOggSoundPlugin::OgreOggSoundPlugin() : 
 mOgreOggSoundFactory(0)
,mOgreOggSoundManager(0)
{

}
//---------------------------------------------------------------------
const String& OgreOggSoundPlugin::getName() const
{
	return sPluginName;
}
//---------------------------------------------------------------------
void OgreOggSoundPlugin::install()
{
	if ( mOgreOggSoundFactory ) return;

	// Create new factory
	mOgreOggSoundFactory = OGRE_NEW_T(OgreOggSoundFactory, Ogre::MEMCATEGORY_GENERAL)();

	// Register
	Root::getSingleton().addMovableObjectFactory(mOgreOggSoundFactory, true);
}
//---------------------------------------------------------------------
void OgreOggSoundPlugin::initialise()
{
	if ( mOgreOggSoundManager ) return;

	//initialise OgreOggSoundManager here
	mOgreOggSoundManager = OGRE_NEW_T(OgreOggSoundManager, Ogre::MEMCATEGORY_GENERAL)();
}
//---------------------------------------------------------------------
void OgreOggSoundPlugin::shutdown()
{
	if ( !mOgreOggSoundManager ) return;

	// shutdown OgreOggSoundManager here
	OGRE_DELETE_T(mOgreOggSoundManager, OgreOggSoundManager, Ogre::MEMCATEGORY_GENERAL);
	mOgreOggSoundManager = 0;
}
//---------------------------------------------------------------------
void OgreOggSoundPlugin::uninstall()
{
	if ( !mOgreOggSoundFactory ) return;

	// unregister
	Root::getSingleton().removeMovableObjectFactory(mOgreOggSoundFactory);

	OGRE_DELETE_T(mOgreOggSoundFactory, OgreOggSoundFactory, Ogre::MEMCATEGORY_GENERAL);
	mOgreOggSoundFactory = 0;
}
