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

#pragma once

#include "OgreOggSoundPrereqs.h"
#include <string>
#include <Ogre.h>
#include <al.h>
#include <vorbis/vorbisfile.h>
#include "OgreOggSoundCallback.h"

/**
 * Number of buffers to use for streaming
 */
#define NUM_BUFFERS 4

namespace OgreOggSound
{
	/**
	 * Structure defining a WAVE sounds format.
	 */
	typedef struct
	{
		DWORD			mFormatChunkSize,
						mDataSize,
						mSampleRate,
						mAvgBytesPerSec,
						mAudioOffset;

		unsigned short	mNumChannels,
						mBlockAlign,
						mBitsPerSample;	

		WORD			mSamples;
		DWORD			mChannelMask;
		GUID			mSubFormat;	
	} WavFormatData;

	/** Action enumeration after a fade has completed.
	@remarks
		Use this to specify what to do on a sound after it has finished fading.	i.e. after fading out pause.
	*/
	enum FadeControl
	{
		NONE	= 0x00,
		PAUSE	= 0x01,
		STOP	= 0x02
	};

	/**
	 * Structure describing an ogg stream
	 */
	struct SOggFile
	{
		char* dataPtr;    // Pointer to the data in memory
		int   dataSize;   // Size of the data
		int   dataRead;   // How much data we have read so far
	};

	/**
	 * class for a sound, provides functions for setting audio properties
	 * on a 3D sound as well as stop/pause/play operations.
	 */
	class _OGGSOUND_EXPORT OgreOggISound : public Ogre::MovableObject
	{

	public:

		/** Opens audio file.
		@remarks
			Abstract function
		 */
		virtual void open(Ogre::DataStreamPtr& fileStream) = 0;
		/** Sets the source object for playback.
		@remarks
			Abstract function
		 */
		virtual void setSource(ALuint& src) = 0;
		/** Plays sound.
		@remarks
			Abstract function
		 */
		virtual void play() = 0;
		/** Pauses sound.
		@remarks
			Abstract function
		 */
		virtual void pause() = 0;
		/** Stops sound.
		@remarks
			Abstract function
		 */
		virtual void stop() = 0;
		/** Sets looping status.
		@remarks
			Abstract function
			@param
				Boolean: true == loop
		 */
		virtual void loop(bool loop){ mLoop = loop; }
		/** Returns play status.
		@remarks
			Checks source state for AL_PLAYING
		 */
		bool isPlaying();
		/** Returns pause status.
		@remarks
			Checks source state for AL_PAUSED
		 */
		bool isPaused();
		/** Returns stop status.
		@remarks
			Checks source state for AL_STOPPED
		 */
		bool isStopped();
		/** Returns position status.
		@remarks
			Returns whether position is local to listener or in world-space
		 */
		bool isRelativeToListener() { return mSourceRelative; }
		/** Sets whether source is given up when stopped.
		@remarks
			This flag indicates that the sound should immediately give up its source if finished playing
			or manually stopped. Useful for infrequent sounds or sounds which only play once. Allows other
			sounds immediate access to a playable source object.
			@param
				giveup true - release source immediately
		 */
		void setGiveUpSourceOnStop(bool giveup=false) { mGiveUpSource=giveup; }
		/** Sets sounds position.
		@param
			pos x/y/z 3D position
		 */
		void setPosition(float posx,float posy, float posz);
		/** Sets sounds position.
		@param
			pos 3D vector position
		 */
		void setPosition(const Ogre::Vector3 &pos);
		/** Sets sounds direction.
		@param
			dir x/y/z 3D direction
		 */
		void setDirection(float dirx, float diry, float dirz);
		/** Sets sounds direction.
		@param
			dir 3D vector direction
		 */
		void setDirection(const Ogre::Vector3 &dir);
		/** Sets sounds velocity.
		@param
			vel 3D x/y/z velocity
		 */
		void setVelocity(float velx, float vely, float velz);
		/** Sets sounds velocity.
		@param
			vel 3D vector velocity
		 */
		void setVelocity(const Ogre::Vector3 &vel);
		/** Sets sounds volume
		@remarks
			Sets sounds current gain value (0..1).
		@param
			gain volume scalar.
		 */
		void setVolume(float gain);
		/** Gets sounds volume
		@remarks
			Gets the current gain value.
		 */
		Ogre::Real getVolume();
		/** Sets sounds maximum attenuation volume
		@remarks
			This value sets the maximum volume level of the sound when closest 
			to the listener.
			@param
				maxGain Volume scalar (0..1)
		 */
		void setMaxVolume(float maxGain);
		/** Sets sounds minimum attenuation volume
		@remarks
			This value sets the minimum volume level of the sound when furthest
			away from the listener.
			@param
				minGain Volume scalar (0..1)
		 */
		void setMinVolume(float minGain);
		/** Sets sounds cone angles
		@remarks
			This value sets the angles of the sound cone used by this sound.
			@param
				insideAngle - angle over which the volume is at maximum
			@param
				outsideAngle - angle over which the volume is at minimum
		 */
		void setConeAngles(float insideAngle=360.f, float outsideAngle=360.f);
		/** Sets sounds outer cone volume
		@remarks
			This value sets the volume level heard at the outer cone angle.
			Usually 0 so no sound heard when not within sound cone.
			@param
				gain Volume scalar (0..1)
		 */
		void setOuterConeVolume(float gain=0.f);
		/** Sets sounds maximum distance
		@remarks
			This value sets the maximum distance at which attenuation is
			stopped. Beyond this distance the volume remains constant.
			@param
				maxDistance Distance.
		*/
		void setMaxDistance(float maxDistance);
		/** Sets sounds minimum distance
		@remarks
			This value sets the minimum distance beyond which the volume 
			is set to maximum.
			@param
				minDistance distance.
		*/
		void setMinDistance(float minDistance);
		/** Sets sounds rolloff factor
		@remarks
			This value sets the rolloff factor applied to the attenuation 
			of the volume over distance. Effectively scales the volume change
			affect.
			@param
				rolloffFactor Factor (>0).
		*/
		void setRolloffFactor(float rolloffFactor);
		/** Sets sounds reference distance
		@remarks
			This value sets the half-volume distance. The distance at which the volume 
			would reduce by half.
			@param
				referenceDistance distance (>0).
		*/
		void setReferenceDistance(float referenceDistance);
		/** Sets sounds pitch
		@remarks
			This affects the playback speed of the sound
			@param
				pitch Pitch (>0).
		*/
		void setPitch(float pitch);	
		/** Sets whether the positional information is relative to the listener
		@remarks
			This specifies whether the sound is attached to listener or in 
			world-space. Default: world-space
			@param
				relative Boolean yes/no.
		*/
		void setRelativeToListener(bool relative);
		/** Gets sounds position
		*/
		Ogre::Vector3& getPosition(){return mPosition;};
		/** Gets the sounds direction
		 */
		Ogre::Vector3& getDirection(){return mDirection;};
		/** Starts a fade in/out of the sound volume
		@remarks
			Triggers a fade in/out of the sounds volume over time.

			@param
				dir Direction to fade. (true=in | false=out)
			@param
				fadeTime Time over which to fade (>0)
			@param
				actionOnCompletion Optional action to perform when fading has finished (default: NONE)
		*/
		void startFade(bool dir, Ogre::Real fadeTime, FadeControl actionOnCompletion=OgreOggSound::NONE);
		/** Returns fade status.
		 */
		bool isFading() { return mFade; }
		/** Updates sund
		@remarks
			Updates sounds position, buffers and state
			@param
				fTime Elapsed frametime.
		*/
		virtual void update(Ogre::Real fTime);
		/** Gets the sounds source
		 */
		ALuint getSource() { return mSource; }
		/** Gets the sounds name
		 */
		const Ogre::String& getName() { return mName; }
		/** Gets the sounds priority
		 */
		Ogre::uint8 getPriority() { return mPriority; }
		/** Sets the sounds priority
		@remarks
			This can be used to specify a priority to the sound which
			will be checked when re-using sources. Higher priorities 
			will tend to keep their sources.
			@param
				priority (0..255)
		 */
		void setPriority(Ogre::uint8 priority) { mPriority=priority; }
		/** Gets movable type string
		@remarks
			Overridden from MovableObject.
		 */
		virtual const Ogre::String& getMovableType(void) const;
		/** Gets bounding box
		@remarks
			Overridden from MovableObject.
		 */
		virtual const Ogre::AxisAlignedBox& getBoundingBox(void) const;
		/** Gets bounding radius
		@remarks
			Overridden from MovableObject.
		 */
		virtual Ogre::Real getBoundingRadius(void) const;
		/** Updates RenderQueue
		@remarks
			Overridden from MovableObject.
		 */
		virtual void _updateRenderQueue(Ogre::RenderQueue *queue);
		/** Notifys object its been attached to a node
		@remarks
			Overridden from MovableObject.
		 */
		virtual void _notifyAttached(Ogre::Node* node, bool isTagPoint=false);
		/** Renderable callback
		@remarks
			Overridden function from MovableObject.
		 */
		virtual void visitRenderables(Ogre::Renderable::Visitor* visitor, bool debugRenderables);
		/** Sets a callback for when sound finishes playing 
		@remarks
			Allows custom functions to be notified when this sound finishes playing.
			@param
				object pointer to this sound
				function pointer to member function
		*/
		template<typename T>
		void setFinishedCallback(T *object, void(T::*function)(OgreOggISound *sound), bool enabled=true)
		{
			mFinishedCB = new OSSCallbackPointer<T>(function, object);
			mFinCBEnabled = enabled;
		}
		/** Sets whether the finished callback is called, if defined.
		@remarks
			Allows users to enable/disable the callback feature as/when required.
			@param
				enabled true=on
		*/
		void setFinishedCallbackEnabled(bool enable)
		{
			mFinCBEnabled = enable;
		}

		/** Sets a callback for when sound loops
		@remarks
			Allows custom functions to be notified when this sound loops.
			@param
				object pointer to this sound
				function pointer to member function
		*/
		template<typename T> void setLoopCallback(T *object, void(T::*function)(OgreOggISound *sound), bool enabled=true)
		{
			mLoopCB = new OSSCallbackPointer<T>(function, object);
			mLoopCBEnabled = enabled;
		}
		/** Sets whether the loop callback is called, if defined.
		@remarks
			Allows users to enable/disable the callback feature as/when required.
			@param
				enable true=on
		*/
		void setLoopCallbackEnabled(bool enable)
		{
			mLoopCBEnabled = enable;
		}

	private:
	
		/** Release all OpenAL objects
		@remarks
			Cleans up buffers and prepares sound for destruction.
		 */
		virtual void _release() = 0;

	protected:

		/** Superclass describing a single sound object.
		 */
		OgreOggISound(const Ogre::String& name);
		/** Superclass destructor.
		 */
		virtual ~OgreOggISound();
		/** Inits source object
		@remarks
			Initialises all the source objects states ready for playback.
		 */
		void _initSource();
		/** Checks transformations for re-sync
		@remarks
			Compares transformations which need re-syncing this update.
			Returns true|false.
		 */
		bool _needUpdate();
		/** Updates a fade
		@remarks
			Updates a fade action.
		 */
		void _updateFade(Ogre::Real fTime=0.f);
		/** Updates audio buffers 
		@remarks
			Abstract function.
		*/
		virtual void _updateAudioBuffers() = 0;
		/** Prefills buffers with audio data.
		@remarks
			Loads audio data from the stream into the predefined data
			buffers and queues them onto the source ready for playback.
		 */
		virtual void _prebuffer() = 0;		
		/** Calculates buffer size and format.
		@remarks
			Calculates a block aligned buffer size of 250ms using
			sound properties.
		 */
		virtual bool _queryBufferInfo()=0;		

		/**
		 * Variables used to fade sound
		 */
		Ogre::Real mFadeTimer;
		Ogre::Real mFadeTime;
		Ogre::Real mFadeInitVol;
		Ogre::Real mFadeEndVol;
		bool mFade;
		FadeControl mFadeEndAction;

		// Ogre resource stream pointer
		Ogre::DataStreamPtr mAudioStream;
		ov_callbacks mOggCallbacks;

		// Callbacks  
		OOSCallback* mLoopCB;
		OOSCallback* mFinishedCB;
		bool mFinCBEnabled;
		bool mLoopCBEnabled;

		size_t mBufferSize;				// Size of audio buffer (250ms)

		/** Sound properties 
		 */
		ALuint mSource;					// OpenAL Source
		Ogre::uint8 mPriority;			// Priority assigned to source 
		Ogre::Vector3 mPosition;		// 3D position
		Ogre::Vector3 mDirection;		// 3D direction
		Ogre::Vector3 mVelocity;		// 3D velocity
		Ogre::Real mGain;				// Current volume
		Ogre::Real mMaxGain;			// Minimum volume
		Ogre::Real mMinGain;			// Maximum volume
		Ogre::Real mMaxDistance;		// Maximum attenuation distance
		Ogre::Real mRolloffFactor;		// Rolloff factor for attenuation
		Ogre::Real mReferenceDistance;	// Half-volume distance for attenuation
		Ogre::Real mPitch;				// Current pitch 
		Ogre::Real mOuterConeGain;		// Outer cone volume
		Ogre::Real mInnerConeAngle;		// Inner cone angle
		Ogre::Real mOuterConeAngle;		// outer cone angle
		Ogre::String mName;				// Sound name
		bool mLoop;						// Loop status
		bool mPlay;						// Play status
		bool mGiveUpSource;				// Flag to indicate whether sound should release its source when stopped
		bool mStream;					// Stream flag
		bool mSourceRelative;			// Relative position flag
		bool mLocalTransformDirty;		// Transformation update flag

		friend class OgreOggSoundManager;
	};
}