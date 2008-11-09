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

#include <OgreRoot.h>

#include "OgreOggSound.h"
#include "OgreOggSoundPlugin.h"

using namespace Ogre;
using namespace OgreOggSound;

const String sPluginName = "OgreOggSound";

//---------------------------------------------------------------------
OgreOggSoundPlugin::OgreOggSoundPlugin()
	:mOgreOggSoundFactory(0)
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
	// Create new factory
	mOgreOggSoundFactory = new OgreOggSoundFactory();

	// Register
	Root::getSingleton().addMovableObjectFactory(mOgreOggSoundFactory, true);
}
//---------------------------------------------------------------------
void OgreOggSoundPlugin::initialise()
{
	//initialise OgreOggSoundManager here
	mOgreOggSoundManager = new OgreOggSoundManager();
}
//---------------------------------------------------------------------
void OgreOggSoundPlugin::shutdown()
{
	// shutdown OgreOggSoundManager here
	delete mOgreOggSoundManager;
	mOgreOggSoundManager = 0;
}
//---------------------------------------------------------------------
void OgreOggSoundPlugin::uninstall()
{
	// unregister
	Root::getSingleton().removeMovableObjectFactory(mOgreOggSoundFactory);

	delete mOgreOggSoundFactory;
	mOgreOggSoundFactory = 0;
}
