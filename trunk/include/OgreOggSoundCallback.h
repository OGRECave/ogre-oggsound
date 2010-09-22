/**
* @file OgreOggSoundCallback.h
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
* @section DESCRIPTION
* 
* Callbacks for detecting various states
*/

#ifndef _OGREOGGSOUND_CALLBACK_H_
#define _OGREOGGSOUND_CALLBACK_H_

#include "OgreOggSoundPrereqs.h"

namespace OgreOggSound
{
	class OgreOggISound;

	//! Callbacks for sound states.
	/** Template class for implementing callbacks which can be attached to sounds.
	@remarks
		Allows member functions to be used as callbacks. Amended from OgreAL, 
		originally written by CaseyB.
	**/
	class _OGGSOUND_EXPORT OOSCallback
	{
	
	public:
	
		virtual ~OOSCallback(){};
		virtual void execute(OgreOggISound* sound) = 0;

	};

	//! Callback template
	template<typename T>
	class OSSCallbackPointer : public OOSCallback
	{

	public:

		typedef void (T::*MemberFunction)(OgreOggISound* sound);

		OSSCallbackPointer() : mUndefined(true){}

		OSSCallbackPointer(MemberFunction func, T* obj) :
			mFunction(func),
			mObject(obj),
			mUndefined(false)
		{}

		virtual ~OSSCallbackPointer(){}

		void execute(OgreOggISound* sound)
		{
			if(!mUndefined)
				(mObject->*mFunction)(sound);
		}

	protected:

		MemberFunction mFunction;
		T* mObject;
		bool mUndefined;

	}; 

} 

#endif	/* _OGREOGGSOUND_CALLBACK_H_ */
