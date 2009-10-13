/**
* @file OgreOggSoundManager.h
* @author  Ian Stangoe
* @version 1.11
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
* Manages the audio library
*/

#ifndef _OGREOGGSOUNDMANAGER_H_
#define _OGREOGGSOUNDMANAGER_H_

#include "OgreOggSoundPrereqs.h"
#include "OgreOggSound.h"
#include "LocklessQueue.h"

#include <map>
#include <string>

#if OGGSOUND_THREADED
#	include <boost/thread/thread.hpp>
#	include <boost/function/function0.hpp>
#	include <boost/thread/recursive_mutex.hpp>
#	include <boost/thread/xtime.hpp>
#endif

namespace OgreOggSound
{
	typedef std::map<std::string, OgreOggISound*> SoundMap;
	typedef std::map<std::string, ALuint> EffectList;
	typedef std::map<ALenum, bool> FeatureList;
	typedef std::list<OgreOggISound*> ActiveList;
	typedef std::deque<ALuint> SourceList;
	
	//! Various sound commands
	enum SOUND_ACTION
	{
		LQ_PLAY,
		LQ_STOP,
		LQ_PAUSE,
		LQ_LOAD,
		LQ_DESTROY,
		LQ_DESTROY_ALL,
		LQ_STOP_ALL,
		LQ_PAUSE_ALL,
		LQ_RESUME_ALL,
		LQ_REACTIVATE
	};

	//! Holds information about a sound action
	struct SoundAction
	{
		OgreOggISound*	mSound;
		SOUND_ACTION	mAction;
		void*			mParams;
	};

	//! Holds information about a create sound request.
	struct cSound
	{
		bool mPrebuffer;
		Ogre::String mFileName;
		ALuint mBuffer;
		Ogre::DataStreamPtr mStream;
	};

	//! Holds information about a static shared audio buffer.
	struct sharedAudioBuffer
	{
		ALuint mAudioBuffer;
		unsigned int mRefCount;

	};

	typedef std::map<std::string, sharedAudioBuffer*> SharedBufferList;

	//! Sound Manager: Manages all sounds for an application
	class _OGGSOUND_EXPORT OgreOggSoundManager : public Ogre::Singleton<OgreOggSoundManager>
	{

	public:

		// Version string
		static const Ogre::String OGREOGGSOUND_VERSION_STRING;

		/** Creates a manager for all sounds within the application.
		 */
		OgreOggSoundManager();
		/** Gets a singleton reference.
		 */
		static OgreOggSoundManager& getSingleton(void);
		/** Gets a singleton reference.
		 */
		static OgreOggSoundManager* getSingletonPtr(void);
		/** Destroys this manager.
		@remarks
			Destroys all sound objects and thread if defined. Cleans up
			all OpenAL objects, buffers and devices and closes down the
			audio device.
		 */
		~OgreOggSoundManager();
		/** Initialises the audio device.
		@remarks
			Attempts to initialise the audio device for sound playback.
			Internally some logging is done to list features supported as
			well as creating a pool of sources from which sounds can be
			attached and played.
			@param deviceName
				Audio device string to open, will use default device if not found.
			@param maxSources
				maximum number of sources to allocate (optional)
			@param queueListSize
				Desired size of queue list (optional | Multi-threaded ONLY)
		 */
		bool init(const std::string &deviceName = "", unsigned int maxSources=100, unsigned int queueListSize=64);
		/** Sets the global volume for all sounds
			@param vol 
				global attenuation for all sounds.
		 */
		void setMasterVolume(ALfloat vol);
		/** Gets number of currently created sounds
		 */
		unsigned int getNumSounds() const { return static_cast<unsigned int>(mSoundMap.size()); }
		/** Gets the current global volume for all sounds
		 */
		ALfloat getMasterVolume();
		/** Creates a single sound object.
		@remarks
			Creates and inits a single sound object, depending on passed
			parameters this function will create a static/streamed sound.
			Each sound must have a unique name within the manager.
			@param name 
				Unique name of sound
			@param file 
				Audio file path string
			@param stream 
				Flag indicating if the sound sound be streamed.
			@param loop 
				Flag indicating if the file should loop.
			@param preBuffer 
				Flag indicating if a source should be attached at creation.
		 */
		OgreOggISound* createSound(const std::string& name,const std::string& file, bool stream = false, bool loop = false, bool preBuffer=false);
		/** Creates a single sound object.
		@remarks
			Plugin specific version of createSound, uses createMovableObject() to instantiate
			a sound automatically registered with the supplied SceneManager, allows OGRE to automatically
			cleanup/manage this sound.
			Each sound must have a unique name within the manager.
			@param scnMgr 
				SceneManager to use to create sound
			@param name 
				Unique name of sound
			@param file 
				Audio file path string
			@param stream 
				Flag indicating if the sound sound be streamed.
			@param loop 
				Flag indicating if the file should loop.
			@param preBuffer 
				Flag indicating if a source should be attached at creation.
		 */
		OgreOggISound* createSound(Ogre::SceneManager& scnMgr, const std::string& name,const std::string& file, bool stream = false, bool loop = false, bool preBuffer=false);
		/** Gets a named sound.
		@remarks
			Returns a named sound object if defined, NULL otherwise.
			@param name 
				Sound name.
		 */
		OgreOggISound *getSound(const std::string& name);
		/** Gets list of created sounds.
		@remarks
			Returns a vector of sound name strings.
		 */
		const Ogre::StringVector getSoundList() const;
		/** Returns whether named sound exists.
		@remarks
			Checks sound map for a named sound.
			@param name 
				Sound name.
		 */
		bool hasSound(const std::string& name);
		/** Stops all currently playing sounds.
		 */
		void stopAllSounds();
		/** Pauses all currently playing sounds.
		 */
		void pauseAllSounds();
		/** Mutes all sounds.
		 */
		void muteAllSounds();
		/** Un mutes all sounds.
		 */
		void unmuteAllSounds();
		/** Resumes all previously playing sounds.
		 */
		void resumeAllPausedSounds();
		/** Destroys all sounds within manager.
		 */
		void destroyAllSounds();
		/** Returns XRAM support status.
		 */
		bool hasXRamSupport() { return mXRamSupport; }
		/** Returns EFX support status.
		 */
		bool hasEFXSupport() { return mEFXSupport; }
		/** Returns EAX support status.
		 */
		bool hasEAXSupport() { return mEAXSupport; }
		/** Destroys a single sound.
		@remarks
			Destroys a single sound object.
			@param name 
				Sound name to destroy.
		 */
		void destroySound(const std::string& name="");
		/** Destroys a single sound.
		@remarks
			Destroys a single sound object.
			@param name 
				Sound name to destroy.
		 */
		void destroySound(OgreOggISound* sound);
		/** Requests a free source object.
		@remarks
			Internal function - SHOULD NOT BE CALLED BY USER CODE
			Retrieves a free source object and attaches it to the
			specified sound object. Internally checks for any currently
			available sources, then checks stopped sounds and finally
			prioritised sounds.
			@param sound 
				Sound pointer.
		 */
		bool _requestSoundSource(OgreOggISound* sound=0);
		/** Release a sounds source.
		@remarks
			Internal function - SHOULD NOT BE CALLED BY USER CODE
			Releases a specified sounds source object back to the system,
			allowing it to be re-used by another sound.
			@param sound 
				Sound pointer.
		 */
		bool _releaseSoundSource(OgreOggISound* sound=0);
		/** Releases a shared audio buffer
		@remarks
			Internal function - SHOULD NOT BE CALLED BY USER CODE
			Each shared audio buffer is reference counted so destruction is handled correctly,
			this function merely decrements the reference count, only destroying when no sounds
			are referencing buffer.
			@param sName
				Name of audio file
			@param buffer
				buffer id
		*/
		bool _releaseSharedBuffer(const Ogre::String& sName, ALuint& buffer);
		/** Registers a shared audio buffer
		@remarks
			Internal function - SHOULD NOT BE CALLED BY USER CODE
			Its possible to share audio buffer data among many sources so this function
			registers an audio buffer as 'sharable', meaning if a the same audio file is
			created more then once, it will simply use the original buffer data instead of
			creating/loading the same data again.
			@param sName
				Name of audio file
			@param buffer
				OpenAL buffer ID holding audio data
		 */
		bool _registerSharedBuffer(const Ogre::String& sName, ALuint& buffer);
		/** Sets distance model.
		@remarks
			Sets the global distance attenuation algorithm used by all
			sounds in the system.
			@param value 
				ALenum value of distance model.
		 */
		void setDistanceModel(ALenum value);
		/** Sets doppler factor.
		@remarks
			Sets the global doppler factor which affects attenuation for
			all sounds
			@param factor 
				Factor scale (>0).
		 */
		void setDopplerFactor(Ogre::Real factor=1.f);
		/** Sets speed of sound.
		@remarks
			Sets the global speed of sound used in the attenuation algorithm,
			affects all sounds.
			@param speed 
				Speed (m/s).
		 */
		void setSpeedOfSound(Ogre::Real speed=363.f);
		/** Gets a list of device strings
		@remarks
			Creates a list of available audio device strings
		 */
		const Ogre::StringVector getDeviceList() const;
		/** Returns pointer to listener.
		 */
		OgreOggListener *getListener(){return mListener;};
		/** Returns number of sources created.
		 */
		int getNumSources() const { return mNumSources; }
		/** Updates system.
		@remarks
			Iterates all sounds and updates them.
			@param fTime 
				Elapsed frametime.
		 */
		void update(Ogre::Real fTime=0.f);
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
		/** Sets XRam buffers.
		@remarks
			Currently defaults to AL_STORAGE_AUTO.
		 */
		void setXRamBuffer(ALsizei numBuffers, ALuint* buffers);
		/** Sets XRam buffers storage mode.
		@remarks
			Should be called before creating any sounds
			Options: AL_STORAGE_AUTOMATIC | AL_STORAGE_HARDWARE | AL_STORAGE_ACCESSIBLE
		 */
		void setXRamBufferMode(ALenum mode);
		/** Sets the distance units of measurement for EFX effects.
		@remarks
			@param unit 
				units(meters).
		 */
		void setEFXDistanceUnits(Ogre::Real unit=3.3f);
		/** Creates a specified EFX filter
		@remarks
			Creates a specified EFX filter if hardware supports it.
			@param eName 
				name for filter.
			@param type 
				see OpenAL docs for available filters.
			@param gain
				see OpenAL docs for available filters.
			@param hfgain 
				see OpenAL docs for available filters.
		 */
		bool createEFXFilter(const std::string& eName, ALint type, ALfloat gain=1.0, ALfloat hfGain=1.0);
		/** Creates a specified EFX effect
		@remarks
			Creates a specified EFX effect if hardware supports it. Optional reverb
			preset structure can be passed which will be applied to the effect. See
			eax-util.h for list of presets.
			@param eName 
				name for effect.
			@param type 
				see OpenAL docs for available effects.
			@param props	
				legacy structure describing a preset reverb effect.
		 */
		bool createEFXEffect(const std::string& eName, ALint type, EAXREVERBPROPERTIES* props=0);
		/** Sets extended properties on a specified sounds source
		@remarks
			Tries to set EFX extended source properties.
			@param sName 
				name of sound.
			@param airAbsorption 
				absorption factor for air.
			@param roomRolloff 
				room rolloff factor.
			@param coneOuterHF 
				cone outer gain factor for High frequencies.
		 */
		bool setEFXSoundProperties(const std::string& eName, Ogre::Real airAbsorption=0.f, Ogre::Real roomRolloff=0.f, Ogre::Real coneOuterHF=0.f);
		/** Sets a specified paremeter on an effect
		@remarks
			Tries to set a parameter value on a specified effect. Returns true/false.
			@param eName 
				name of effect.
			@param effectType 
				see OpenAL docs for available effects.
			@param attrib 
				parameter value to alter.
			@param param 
				float value to set.
		 */
		bool setEFXEffectParameter(const std::string& eName, ALint effectType, ALenum attrib, ALfloat param);
		/** Sets a specified paremeter on an effect
		@remarks
			Tries to set a parameter value on a specified effect. Returns true/false.
			@param eName 
				name of effect.
			@param type 
				see OpenAL docs for available effects.
			@param attrib 
				parameter value to alter.
			@param params 
				vector pointer of float values to set.
		 */
		bool setEFXEffectParameter(const std::string& eName, ALint type, ALenum attrib, ALfloat* params=0);
		/** Sets a specified paremeter on an effect
		@remarks
			Tries to set a parameter value on a specified effect. Returns true/false.
			@param eName 
				name of effect.
			@param type 
				see OpenAL docs for available effects.
			@param attrib 
				parameter value to alter.
			@param param 
				integer value to set.
		 */
		bool setEFXEffectParameter(const std::string& eName, ALint type, ALenum attrib, ALint param);
		/** Sets a specified paremeter on an effect
		@remarks
			Tries to set a parameter value on a specified effect. Returns true/false.
			@param eName 
				name of effect.
			@param type 
				see OpenAL docs for available effects.
			@param attrib 
				parameter value to alter.
			@param params 
				vector pointer of integer values to set.
		 */
		bool setEFXEffectParameter(const std::string& eName, ALint type, ALenum attrib, ALint* params=0);
		/** Gets the maximum number of Auxiliary Effect slots per source
		@remarks
			Determines how many simultaneous effects can be applied to
			any one source object
		 */
		int getNumberOfSupportedEffectSlots();
		/** Gets the number of currently created Auxiliary Effect slots
		@remarks
			Returns number of slots craeted and available for effects/filters.
		 */
		int getNumberOfCreatedEffectSlots();
		/** Creates a specified EFX filter
		@remarks
			Creates a specified EFX filter if hardware supports it.
			@param eName 
				name for filter. type see OpenAL docs for available filter types.
		 */
		bool createEFXSlot();
		/** Attaches an effect to a sound
		@remarks
			Currently sound must have a source attached prior to this call.
			@param sName 
				name of sound
			@param slot 
				slot ID
			@param effect 
				name of effect as defined when created
			@param filter 
				name of filter as defined when created
		 */
		bool attachEffectToSound(const std::string& sName, ALuint slot, const Ogre::String& effect="", const Ogre::String& filter="");
		/** Attaches a filter to a sound
		@remarks
			Currently sound must have a source attached prior to this call.
			@param sName 
				name of sound
			@param filter 
				name of filter as defined when created
		 */
		bool attachFilterToSound(const std::string& sName, const Ogre::String& filter="");
		/** Detaches all effects from a sound
		@remarks
			Currently sound must have a source attached prior to this call.
			@param sName 
				name of sound
			@param slotID 
				slot ID
		 */
		bool detachEffectFromSound(const std::string& sName, ALuint slotID);
		/** Detaches all filters from a sound
		@remarks
			Currently sound must have a source attached prior to this call.
			@param sName 
				name of sound
		 */
		bool detachFilterFromSound(const std::string& sName);
		/** Returns whether a specified effect is supported
			@param effectID 
				OpenAL effect/filter id. (AL_EFFECT... | AL_FILTER...)
		 */
		bool isEffectSupported(ALint effectID);
		/** Gets recording device
		 */
		OgreOggSoundRecord* getRecorder() { return mRecorder; }
		/** Returns whether a capture device is available
		 */
		bool isRecordingAvailable() const;
		/** Creates a recordable object
		 */
		OgreOggSoundRecord* createRecorder();
#endif

#if OGGSOUND_THREADED

		/** Pushes a sound action request onto the queue
		@remarks
			Internal function - SHOULD NOT BE CALLED BY USER CODE!
			Sound actions are queued through the manager to be operated on in an efficient and
			non-blocking manner, this function adds a request to the list to be processed.
			@param sound
				Sound object to perform action upon
			@param action
				Action to perform.
		*/
		void _requestSoundAction(const SoundAction& action);
		/** Processes queued sound actions.
		@remarks
			Presently executes a maximum of 5 actions in a single iteration.
		 */
		void _processQueuedSounds(void);
		/** Updates all sound buffers.
		@remarks
			Iterates all sounds and updates their buffers.
		 */
		void _updateBuffers();

		boost::recursive_mutex mMutex;

		LocklessQueue<SoundAction>* mActionsList;
		LocklessQueue<SoundAction>* mDelayedActionsList;

		static::boost::thread *mUpdateThread;
		static bool mShuttingDown;
		
		/** Threaded function for streaming updates
		@remarks
			Optional threading function specified in OgreOggPreReqs.h.
			Implemented to handle updating of streamed audio buffers
			independently of main game thread, unthreaded streaming
			would be disrupted by any pauses or large frame lags, due to
			the fact that OpenAL itself runs in its own thread. If the audio
			buffers aren't constantly re-filled the sound will be automatically
			stopped by OpenAL. Static sounds do not suffer this problem because all the
			audio data is preloaded into memory.
		 */
		static void threadUpdate()
		{
			while(!mShuttingDown)
			{
				OgreOggSoundManager::getSingleton()._processQueuedSounds();
				OgreOggSoundManager::getSingleton()._updateBuffers();
				boost::this_thread::sleep(boost::posix_time::millisec(10));
			}
		}

#endif
	private:

		/** Implementation of sound loading
		@param sound
			sound pointer.
		@param file
			name of sound file.
		@param buf
			OpenAL shared buffer index.
		@param prebuffer
			Prebuffer flag.
		*/
		void _loadSoundImpl(OgreOggISound* sound, const Ogre::String& file, ALuint buf, bool prebuffer);
		/** Implementation of sound loading
		@param sound
			sound pointer.
		@param stream
			File stream pointer.
		@param prebuffer
			Prebuffer flag.
		*/
		void _loadSoundImpl(OgreOggISound* sound, Ogre::DataStreamPtr& stream, bool prebuffer);
		/** Creates a single sound object.
		@remarks
			Creates and inits a single sound object, depending on passed
			parameters this function will create a static/streamed sound.
			Each sound must have a unique name within the manager.
			@param name 
				Unique name of sound
			@param file 
				Audio file path string
			@param stream 
				Flag indicating if the sound sound be streamed.
			@param loop 
				Flag indicating if the file should loop.
			@param preBuffer 
				Flag indicating if a source should be attached at creation.
		 */
		OgreOggISound* _createSoundImpl(const std::string& name,const std::string& file, bool stream = false, bool loop = false, bool preBuffer=false);
		/** Creates a single sound object.
		@remarks
			Plugin specific version of createSound, uses createMovableObject() to instantiate
			a sound automatically registered with the supplied SceneManager, allows OGRE to automatically
			cleanup/manage this sound.
			Each sound must have a unique name within the manager.
			@param scnMgr 
				SceneManager to use to create sound
			@param name 
				Unique name of sound
			@param file 
				Audio file path string
			@param stream 
				Flag indicating if the sound sound be streamed.
			@param loop 
				Flag indicating if the file should loop.
			@param preBuffer 
				Flag indicating if a source should be attached at creation.
		 */
		OgreOggISound* _createSoundImpl(Ogre::SceneManager& scnMgr, const std::string& name,const std::string& file, bool stream = false, bool loop = false, bool preBuffer=false);
		/** Destroys a single sound.
		@remarks
			Destroys a single sound object.
			@param sName 
				Sound name to destroy.
		 */
		void _destroySoundImpl(const Ogre::String& sName="");
		/** Destroys a single sound.
		@remarks
			Destroys a single sound object.
			@param sound
				Sound object to destroy.
		 */
		void _destroySoundImpl(OgreOggISound* sound=0);
		/** Destroys a single sound.
		@remarks
			Destroys a single sound object.
			@param sound
				Sound to destroy.
		 */
		void _releaseSoundImpl(OgreOggISound* sound=0);
		/** Removes references of a sound from all possible internal lists.
		@remarks
			Various lists exist to manage numerous states of a sound, this
			function exists to remove a sound object from any/all lists it has
			previously been added to. 
			@param sound
				Sound to destroy.
		 */
		void _removeFromLists(OgreOggISound* sound=0);
		/** Stops all currently playing sounds.
		 */
		void _stopAllSoundsImpl();
		/** Pauses all currently playing sounds.
		 */
		void _pauseAllSoundsImpl();
		/** Resumes all previously playing sounds.
		 */
		void _resumeAllPausedSoundsImpl();
		/** Destroys all sounds.
		 */
		void _destroyAllSoundsImpl();
		/** Creates a pool of OpenAL sources for playback.
		@remarks
			Attempts to create a pool of source objects which allow
			simultaneous audio playback. The number of sources will be
			clamped to either the hardware maximum or [mMaxSources]
			whichever comes first.
		 */
		int _createSourcePool();
		/** Gets a shared audio buffer
		@remarks
			Returns a previously loaded shared buffer reference if available.
			NOTE:- Increments a reference count so releaseSharedBuffer() must be called
			when buffer is no longer used.
			@param sName
				Name of audio file
		 */
		ALuint _getSharedBuffer(const Ogre::String& sName);
		/** Releases all sounds and buffers
		@remarks
			Release all sounds and their associated OpenAL objects
			from the system.
		 */
		void _releaseAll();
		/** Checks and Logs a supported feature list
		@remarks
			Queries OpenAL for various supported features and lists
			them with the LogManager.
		 */
		void _checkFeatureSupport();

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32

		/** Checks for EFX hardware support
		 */
		bool _checkEFXSupport();
		/** Checks for XRAM hardware support
		 */
		bool _checkXRAMSupport();
		/** Checks for EAX effect support
		 */
		void _determineAuxEffectSlots();
		/** Gets a specified EFX filter
		@param fName 
			name of filter as defined when created.
		 */
		ALuint _getEFXFilter(const std::string& fName);
		/** Gets a specified EFX Effect
		@param eName 
			name of effect as defined when created.
		 */
		ALuint _getEFXEffect(const std::string& eName);
		/** Gets a specified EFX Effect slot
		@param slotID 
			index of auxiliary effect slot
		 */
		ALuint _getEFXSlot(int slotID=0);
		/** Sets EAX reverb properties using a specified present
		@param pEFXEAXReverb 
			pointer to converted EFXEAXREVERBPROPERTIES structure object
		@param uiEffect 
			effect ID
		 */
		bool _setEAXReverbProperties(EFXEAXREVERBPROPERTIES *pEFXEAXReverb, ALuint uiEffect);
		/** Attaches a created effect to an Auxiliary slot
		@param slot 
			slot ID
		@param effect 
			effect ID
		 */
		bool _attachEffectToSlot(ALuint slot, ALuint effect);
#endif
		/** Re-activates any sounds which had their source stolen.
		 */
		void _reactivateQueuedSounds();
		/** Re-activates any sounds which had their source stolen, implementation methods.
		@remarks
			When all sources are in use the sounds begin to give up
			their source objects to higher priority sounds. When this
			happens the lower priority sound is queued ready to re-play
			when a source becomes available again, this function checks
			this queue and tries to re-play those sounds. Only affects
			sounds which were originally playing when forced to give up
			their source object.
		 */
		void _reactivateQueuedSoundsImpl();
		/** Enumerates audio devices.
		@remarks
			Gets a list of audio device available.
		 */
		void _enumDevices();

		/**
		 * OpenAL device objects
		 */
		ALCdevice* mDevice;						// OpenAL device
		ALCcontext* mContext;					// OpenAL context

		ALfloat	mOrigVolume;					// Used to revert volume after a mute

		/** Sound lists
		 */
		SoundMap mSoundMap;						// Map of all sounds
		ActiveList mActiveSounds;				// list of sounds currently active
		ActiveList mPausedSounds;				// list of sounds currently paused
		ActiveList mSoundsToReactivate;			// list of sounds that need re-activating when sources become available
		ActiveList mWaitingSounds;				// list of sounds that need playing when sources become available
		SourceList mSourcePool;					// List of available sources
		FeatureList mEFXSupportList;			// List of supported EFX effects by OpenAL ID
		SharedBufferList mSharedBuffers;		// List of shared static buffers

		/** Manager instance
		 */
		static OgreOggSoundManager *pInstance;	// OgreOggSoundManager instance pointer
		ALCchar* mDeviceStrings;				// List of available devices strings
		unsigned int mNumSources;				// Number of sources available for sounds
		unsigned int mMaxSources;				// Maximum Number of sources to allocate

		OgreOggSoundRecord* mRecorder;			// recorder object

		//! sorts sound list by distance
		struct _sortNearToFar;
		//! sorts sound list by distance
		struct _sortFarToNear;

		/**	EFX Support
		*/
		bool mEFXSupport;						// EFX present flag

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32

		// Effect objects
		LPALGENEFFECTS alGenEffects;
		LPALDELETEEFFECTS alDeleteEffects;
		LPALISEFFECT alIsEffect;
		LPALEFFECTI alEffecti;
		LPALEFFECTIV alEffectiv;
		LPALEFFECTF alEffectf;
		LPALEFFECTFV alEffectfv;
		LPALGETEFFECTI alGetEffecti;
		LPALGETEFFECTIV alGetEffectiv;
		LPALGETEFFECTF alGetEffectf;
		LPALGETEFFECTFV alGetEffectfv;

		//Filter objects
		LPALGENFILTERS alGenFilters;
		LPALDELETEFILTERS alDeleteFilters;
		LPALISFILTER alIsFilter;
		LPALFILTERI alFilteri;
		LPALFILTERIV alFilteriv;
		LPALFILTERF alFilterf;
		LPALFILTERFV alFilterfv;
		LPALGETFILTERI alGetFilteri;
		LPALGETFILTERIV alGetFilteriv;
		LPALGETFILTERF alGetFilterf;
		LPALGETFILTERFV alGetFilterfv;

		// Auxiliary slot object
		LPALGENAUXILIARYEFFECTSLOTS alGenAuxiliaryEffectSlots;
		LPALDELETEAUXILIARYEFFECTSLOTS alDeleteAuxiliaryEffectSlots;
		LPALISAUXILIARYEFFECTSLOT alIsAuxiliaryEffectSlot;
		LPALAUXILIARYEFFECTSLOTI alAuxiliaryEffectSloti;
		LPALAUXILIARYEFFECTSLOTIV alAuxiliaryEffectSlotiv;
		LPALAUXILIARYEFFECTSLOTF alAuxiliaryEffectSlotf;
		LPALAUXILIARYEFFECTSLOTFV alAuxiliaryEffectSlotfv;
		LPALGETAUXILIARYEFFECTSLOTI alGetAuxiliaryEffectSloti;
		LPALGETAUXILIARYEFFECTSLOTIV alGetAuxiliaryEffectSlotiv;
		LPALGETAUXILIARYEFFECTSLOTF alGetAuxiliaryEffectSlotf;
		LPALGETAUXILIARYEFFECTSLOTFV alGetAuxiliaryEffectSlotfv;

		/**	XRAM Support
		*/
		typedef ALboolean (__cdecl *LPEAXSETBUFFERMODE)(ALsizei n, ALuint *buffers, ALint value);
		typedef ALenum    (__cdecl *LPEAXGETBUFFERMODE)(ALuint buffer, ALint *value);

		LPEAXSETBUFFERMODE mEAXSetBufferMode;
		LPEAXGETBUFFERMODE mEAXGetBufferMode;
#endif
		/**	EAX Support
		*/
		bool mEAXSupport;						// EAX present flag
		int mEAXVersion;						// EAX version ID

		bool mXRamSupport;

		EffectList mFilterList;					// List of EFX filters
		EffectList mEffectList;					// List of EFX effects
		SourceList mEffectSlotList;				// List of EFX effect slots

		ALint mNumEffectSlots;					// Number of effect slots available
		ALint mNumSendsPerSource;				// Number of aux sends per source

		ALenum	mXRamSize,
				mXRamFree,
				mXRamAuto,
				mXRamHardware,
				mXRamAccessible,
				mCurrentXRamMode;

		ALint	mXRamSizeMB,
				mXRamFreeMB;

		/**	Listener pointer
		 */
		OgreOggListener *mListener;				// Listener object

		friend class OgreOggSoundFactory;
	};
}

#endif /* _OGREOGGSOUNDMANAGER_H_ */

