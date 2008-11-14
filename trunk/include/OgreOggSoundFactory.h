/*---------------------------------------------------------------------------*\
** This source file is part of OgreOggSound, an OpenAL wrapper library for   
** use with the Ogre Rendering Engine.										 
**                                                                           
** Copyright 2008 Ian Stangoe & Eric Boissard								 
**                                                                           
** OgreOggSound is free software: you can redistribute it and/or modify		  
** it under the terms of the GNU Lesser General Public License as published	 
** by the Free Software Foundation, either version 3 of the License, or		 
** (at your option) any later version.										 
**																			 
** OgreOggSound is distributed in the hope that it will be useful,			 
** but WITHOUT ANY WARRANTY; without even the implied warranty of			 
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the			 
** GNU Lesser General Public License for more details.						 
**																			 
** You should have received a copy of the GNU Lesser General Public License	 
** along with OgreOggSound.  If not, see <http://www.gnu.org/licenses/>.	 
\*---------------------------------------------------------------------------*/

#ifndef __OGREOGGSOUNDFactory_H__
#define __OGREOGGSOUNDFactory_H__

#include <Ogre.h>

#include "OgreOggSound.h"

namespace OgreOggSound 
{
	/** MovableFactory for creating Sound instances */
	class _OGGSOUND_EXPORT OgreOggSoundFactory : public Ogre::MovableObjectFactory
	{

	protected:
		Ogre::MovableObject* createInstanceImpl( const Ogre::String& name, const Ogre::NameValuePairList* params);

	public:
		OgreOggSoundFactory() {}
		~OgreOggSoundFactory() {}

		static Ogre::String FACTORY_TYPE_NAME;

		const Ogre::String& getType(void) const;
		void destroyInstance( Ogre::MovableObject* obj);

	};
}

#endif
