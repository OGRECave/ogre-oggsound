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

#include "OgreOggListener.h"
#include "OgreOggSound.h"
#include <OgreMovableObject.h>

namespace OgreOggSound
{
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggListener::setPosition(ALfloat x, ALfloat y, ALfloat z)
	{
		mPosition.x = x;
		mPosition.y = y;
		mPosition.z = z;
		alListener3f(AL_POSITION,x,y,z);
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggListener::setPosition(const Ogre::Vector3 &pos)
	{
		mPosition = pos;
		alListener3f(AL_POSITION,pos.x,pos.y,pos.z);
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggListener::setVelocity(float velx, float vely, float velz)
	{
		mVelocity.x = velx;
		mVelocity.y = vely;
		mVelocity.z = velz;
		alListener3f(AL_VELOCITY, velx, vely, velz);
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggListener::setVelocity(const Ogre::Vector3 &vel)
	{
		mVelocity = vel;	
		alListener3f(AL_VELOCITY, vel.x, vel.y, vel.z);
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggListener::setOrientation(ALfloat x,ALfloat y,ALfloat z,ALfloat upx,ALfloat upy,ALfloat upz)
	{
		mOrientation[0] = x;
		mOrientation[1] = y;
		mOrientation[2] = z;
		mOrientation[3] = upx;
		mOrientation[4] = upy;
		mOrientation[5] = upz;	
		alListenerfv(AL_ORIENTATION,mOrientation);
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggListener::setOrientation(const Ogre::Quaternion &q)
	{
		Ogre::Vector3 vDirection = q.zAxis();
		Ogre::Vector3 vUp = q.yAxis();

		mOrientation[0] = -vDirection.x;
		mOrientation[1] = -vDirection.y;
		mOrientation[2] = -vDirection.z;
		mOrientation[3] = vUp.x;
		mOrientation[4] = vUp.y;
		mOrientation[5] = vUp.z;	
		alListenerfv(AL_ORIENTATION,mOrientation);	
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggListener::update()
	{
		if(mLocalTransformDirty)
		{
			if ( mParentNode )
			{
				setPosition(mParentNode->_getDerivedPosition());
				setOrientation(mParentNode->_getDerivedOrientation());			 
			}
			mLocalTransformDirty=false;
		}
	}
	/*/////////////////////////////////////////////////////////////////*/
	const Ogre::String& OgreOggListener::getMovableType(void) const
	{
		static Ogre::String typeName = "OgreOggListener";
		return typeName;
	}
	/*/////////////////////////////////////////////////////////////////*/
	const Ogre::AxisAlignedBox& OgreOggListener::getBoundingBox(void) const
	{
		static Ogre::AxisAlignedBox aab;
		return aab;
	}
	/*/////////////////////////////////////////////////////////////////*/
	Ogre::Real OgreOggListener::getBoundingRadius(void) const
	{
		return 0;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggListener::_updateRenderQueue(Ogre::RenderQueue *queue)
	{
		return;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggListener::visitRenderables(Ogre::Renderable::Visitor* visitor, bool debugRenderables)
	{
		return;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggListener::_notifyAttached(Ogre::Node* node, bool isTagPoint)
	{
		// Call base class notify
		Ogre::MovableObject::_notifyAttached(node, isTagPoint);

		// Immediately set position/orientation when attached
		if (mParentNode)
		{
			setPosition(mParentNode->_getDerivedPosition());
			setOrientation(mParentNode->_getDerivedOrientation());
		}

		return;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggListener::_notifyMoved(void) 
	{ 
		// Call base class notify
		Ogre::MovableObject::_notifyMoved();

		mLocalTransformDirty=true; 
	}
}

	