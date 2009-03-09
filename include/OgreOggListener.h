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

#pragma once

#include "OgreOggSoundPrereqs.h"
#include <OgreVector3.h>
#include <OgreMovableObject.h>

namespace OgreOggSound
{
	/**
	 * Handles the listener from which positioning is calculated
	 */
	class  _OGGSOUND_EXPORT OgreOggListener : public Ogre::MovableObject
	{

	public:		

		/** Constructor
		@remarks
			Creates a listener object to act as the ears of the user. 
		 */
		OgreOggListener(): mLocalTransformDirty(false), mPosition(Ogre::Vector3::ZERO)
		{
			for (int i=0; i<6; i++ ) mOrientation[i]=0;		
		};
		/** Sets the listeners volume.
		@remarks
			This sets the global master gain for all sounds heard through
			the listener.
			@param
				vol master gain for all sounds. (MUST be positive)
		 */
		void setListenerVolume(ALfloat vol=1.0); 
		/** Gets the listeners volume.
		 */
		ALfloat getListenerVolume();
		/** Sets the position of the listener.
		@remarks
			Sets the 3D position of the listener. This is a manual method,
			if attached to a SceneNode this will automatically be handled 
			for you.
			@param
				x/y/z position.
		*/
		void setPosition(ALfloat x,ALfloat y, ALfloat z);
		/** Sets the position of the listener.
		@remarks
			Sets the 3D position of the listener. This is a manual method,
			if attached to a SceneNode this will automatically be handled 
			for you.
			@param
				pos Vector position.
		*/
		void setPosition(const Ogre::Vector3 &pos);
		/** Gets the position of the listener.
		*/
		const Ogre::Vector3& getPosition() { return mPosition; }
		/** Sets the orientation of the listener.
		@remarks
			Sets the 3D orientation of the listener. This is a manual method,
			if attached to a SceneNode this will automatically be handled 
			for you.
			@param
				x/y/z direction.
			@param
				upx/upy/upz up.
		 */
		void setOrientation(ALfloat x, ALfloat y, ALfloat z, ALfloat upx, ALfloat upy, ALfloat upz);
		/** Sets the orientation of the listener.
		@remarks
			Sets the 3D orientation of the listener. This is a manual method,
			if attached to a SceneNode this will automatically be handled 
			for you.
			@param
				q Orientation quaternion.
		 */
		void setOrientation(const Ogre::Quaternion &q);
		/** Gets the orientation of the listener.
		*/
		Ogre::Vector3 getOrientation() { return Ogre::Vector3(mOrientation[0],mOrientation[1],mOrientation[2]); }
		/** Updates the listener.
		@remarks
			Handles positional updates to the listener either automatically
			through the SceneGraph attachment or manually using the 
			provided functions.
		 */
		void update();
		/** Gets the movable type string for this object.
		@remarks
			Overridden function from MovableObject, returns a 
			Sound object string for identification.
		 */
		virtual const Ogre::String& getMovableType(void) const;
		/** Gets the bounding box of this object.
		@remarks
			Overridden function from MovableObject, provides a
			bounding box for this object.
		 */
		virtual const Ogre::AxisAlignedBox& getBoundingBox(void) const;
		/** Gets the bounding radius of this object.
		@remarks
			Overridden function from MovableObject, provides the
			bounding radius for this object.
		 */
		virtual Ogre::Real getBoundingRadius(void) const;
		/** Updates the RenderQueue for this object
		@remarks
			Overridden function from MovableObject.
		 */
		virtual void _updateRenderQueue(Ogre::RenderQueue *queue);
		/** Renderable callback
		@remarks
			Overridden function from MovableObject.
		 */
		virtual void visitRenderables(Ogre::Renderable::Visitor* visitor, bool debugRenderables);
		/** Attach callback
		@remarks
			Overridden function from MovableObject.
		 */
		virtual void _notifyAttached(Ogre::Node* node, bool isTagPoint=false);
		/** Moved callback
		@remarks
			Overridden function from MovableObject.
		 */
		virtual void _notifyMoved(void);
		
	private:

		/**
		 * Positional variables
		 */
		Ogre::Vector3 mPosition;	// 3D position
		float mOrientation[6];		// 3D orientation
		bool mLocalTransformDirty;	// Dirty transforms flag
	};
}