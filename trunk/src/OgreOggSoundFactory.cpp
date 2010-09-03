/**
* @file OgreOggSoundFactory.cpp
* @author  Ian Stangoe
* @version 1.17
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

#include <Ogre.h>

#include "OgreOggSound.h"
#include "OgreOggSoundFactory.h"

using namespace Ogre;
using namespace OgreOggSound;

String OgreOggSoundFactory::FACTORY_TYPE_NAME = "OgreOggISound";
//-----------------------------------------------------------------------
const String& OgreOggSoundFactory::getType(void) const
{
	return FACTORY_TYPE_NAME;
}
//-----------------------------------------------------------------------
MovableObject* OgreOggSoundFactory::createInstanceImpl( const String& name, const NameValuePairList* params)
{
	String fileName;
	bool loop = false;
	bool stream = false;
	bool preBuffer = false;
	SceneManager* scnMgr = 0;

	if (params != 0)
	{
		NameValuePairList::const_iterator fileNameIterator = params->find("fileName");
		if (fileNameIterator != params->end())
		{
			// Get fontname
			fileName = fileNameIterator->second;
		}

		NameValuePairList::const_iterator loopIterator = params->find("loop");
		if (loopIterator != params->end())
		{
			// Get fontname
			loop = StringUtil::match(loopIterator->second,"true",false);
		}

		NameValuePairList::const_iterator streamIterator = params->find("stream");
		if (streamIterator != params->end())
		{
			// Get fontname
			stream = StringUtil::match(streamIterator->second,"true",false);
		}

		NameValuePairList::const_iterator preBufferIterator = params->find("preBuffer");
		if (preBufferIterator != params->end())
		{
			// Get fontname
			preBuffer = StringUtil::match(preBufferIterator->second,"true",false);
		}

		NameValuePairList::const_iterator sManIterator = params->find("sceneManagerName");
		if (sManIterator != params->end())
		{
			// Get fontname
			scnMgr = Ogre::Root::getSingletonPtr()->getSceneManager(sManIterator->second);
		}

		// when no caption is set
		if ( !scnMgr || name == StringUtil::BLANK || fileName == StringUtil::BLANK )
		{
			OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
				"'name & fileName & sceneManagerName' parameters required when constructing an OgreOggISound.",
				"OgreOggSoundFactory::createInstance");
		}

		return OgreOggSoundManager::getSingletonPtr()->_createSoundImpl(*scnMgr, name, fileName, stream, loop, preBuffer);
	}
	else
		return OgreOggSoundManager::getSingletonPtr()->_createListener();

	return 0;
}
//-----------------------------------------------------------------------
void OgreOggSoundFactory::destroyInstance( MovableObject* obj)
{
	if ( dynamic_cast<OgreOggListener*>(obj) )
		// destroy the listener
		OgreOggSoundManager::getSingletonPtr()->_destroyListener();
	else
		// destroy the sound
		OgreOggSoundManager::getSingletonPtr()->_releaseSoundImpl(static_cast<OgreOggISound*>(obj));
}
//-----------------------------------------------------------------------
