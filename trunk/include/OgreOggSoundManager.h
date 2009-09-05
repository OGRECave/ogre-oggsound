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
#include "OgreOggISound.h"
#include "OgreOggListener.h"
#include "OgreOggSoundRecord.h"

#include <map>
#include <string>

#if OGGSOUND_THREADED
#	include <boost/thread/thread.hpp>
#	include <boost/thread/recursive_mutex.hpp>
#	include <boost/thread/xtime.hpp>
#endif

namespace OgreOggSound
{
	typedef std::map<std::string, OgreOggISound*> SoundMap;
	typedef std::map<std::string, ALuint> EffectList;
	typedef std::map<ALenum, bool> FeatureList;
	typedef std::vector<OgreOggISound*> ActiveList;
	typedef std::vector<ALuint> SourceList;

	/** Structure holding information for a threaded file open operation.
	*/
	struct delayedFileOpen
	{
		bool mPrebuffer;
		Ogre::DataStreamPtr mFile;
		Ogre::String mFileName;
		ALuint mBuffer;
		OgreOggISound* mSound;
	};

	/** Structure holding information for a static shared audio buffer.
	*/
	struct sharedAudioBuffer
	{
		ALuint mAudioBuffer;
		unsigned int mRefCount;

	};

	typedef std::deque<delayedFileOpen*> FileOpenList;
	typedef std::map<std::string, sharedAudioBuffer*> SharedBufferList;

	/** Enumeration describing action to perform once a file has been loaded
	 */
	enum DELAYED_ACTION
	{
		DA_PLAY,
		DA_STOP,
		DA_PAUSE
	};

	/** Handles ALL sounds
	 */
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
		 */
		bool init(const std::string &deviceName = "", unsigned int maxSources=100);
		/** Creates a pool of OpenAL sources for playback.
		@remarks
			Attempts to create a pool of source objects which allow
			simultaneous audio playback. The number of sources will be
			clamped to either the hardware maximum or [mMaxSources]
			whichever comes first.
		 */
		int createSourcePool();
		/** Sets the global volume for all sounds
			@param
				vol global attenuation for all sounds.
		 */
		void setMasterVolume(ALfloat vol);
		/** Gets the current global volume for all sounds
		 */
		ALfloat getMasterVolume();
		/** Creates a single sound object.
		@remarks
			Creates and inits a single sound object, depending on passed
			parameters this function will create a static/streamed sound.
			Each sound must have a unique name within the manager.
			@param
				name Unique name of sound
			@param
				file Audio file path string
			@param
				stream Flag indicating if the sound sound be streamed.
			@param
				loop Flag indicating if the file should loop.
			@param
				preBuffer Flag indicating if a source should be attached at creation.
		 */
		OgreOggISound* createSound(const std::string& name,const std::string& file, bool stream = false, bool loop = false, bool preBuffer=false);
		/** Creates a single sound object.
		@remarks
			Plugin specific version of createSound, uses createMovableObject() to instantiate
			a sound automatically registered with the supplied SceneManager, allows OGRE to automatically
			cleanup/manage this sound.
			Each sound must have a unique name within the manager.
			@param
				scnMgr SceneManager to use to create sound
			@param
				name Unique name of sound
			@param
				file Audio file path string
			@param
				stream Flag indicating if the sound sound be streamed.
			@param
				loop Flag indicating if the file should loop.
			@param
				preBuffer Flag indicating if a source should be attached at creation.
		 */
		OgreOggISound* createSound(Ogre::SceneManager& scnMgr, const std::string& name,const std::string& file, bool stream = false, bool loop = false, bool preBuffer=false);
		/** Gets a named sound.
		@remarks
			Returns a named sound object if defined, NULL otherwise.
			@param
				name Sound name.
		 */
		OgreOggISound *getSound(const std::string& name);
		/** Gets list of created sounds.
		@remarks
			Returns a vector of sound name strings.
		 */
		Ogre::StringVector getSoundList();
		/** Returns whether named sound exists.
		@remarks
			Checks sound map for a named sound.
			@param
				name Sound name.
		 */
		bool hasSound(const std::string& name);
		/** Plays a sound.
		@remarks
			NOTE:- it is essential this function is used to play a sound when using BOOST Threads.
			Accessing a sound directly and calling its functions by-passes the thread mutex and causes
			audio artefacts and memory curruption. (Non multi-threaded does not have this problem.)
		 */
		void playSound(const Ogre::String& sName);
		/** Pauses a sound.
		@remarks
			NOTE:- it is essential this function is used to pause a sound when using BOOST Threads.
			Accessing a sound directly and calling its functions by-passes the thread mutex and causes
			audio artefacts and memory curruption. (Non multi-threaded does not have this problem.)
		 */
		void pauseSound(const Ogre::String& sName);
		/** Stops a sound.
		@remarks
			NOTE:- it is essential this function is used to stop a sound when using BOOST Threads.
			Accessing a sound directly and calling its functions by-passes the thread mutex and causes
			audio artefacts and memory curruption. (Non multi-threaded does not have this problem.)
		 */
		void stopSound(const Ogre::String& sName);
		/** Sets a sounds current time position.
		@param
			sName - Name of sound
		@param
			time - Time in seconds to skip to, NOTE:- Value will be wrapped
		*/
		void setSoundCurrentTime(const Ogre::String& sName, Ogre::Real time);
		/** Fades a sound.
		@remarks
			NOTE:- it is essential this function is used to fade a sound when using BOOST Threads.
			Accessing a sound directly and calling its functions by-passes the thread mutex and causes
			audio artefacts and memory curruption. (Non multi-threaded does not have this problem.)
		 */
		void fadeSound(const Ogre::String& sName, bool dir, Ogre::Real fTime, FadeControl actionOnComplete);
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
		@remarks
			Destroys all sound objects created by this manager.
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
			@param
				name Sound name to destroy.
		 */
		void destroySound(const std::string& name="");
		/** Updates all sound buffers.
		@remarks
			Iterates all sounds and updates their buffers.
		 */
		void updateBuffers();
		/** Requests a free source object.
		@remarks
			Retrieves a free source object and attaches it to the
			specified sound object. Internally checks for any currently
			available sources, then checks stopped sounds and finally
			prioritised sounds.
			@param
				sound Sound pointer.
		 */
		bool requestSoundSource(OgreOggISound* sound=0);
		/** Release a sounds source.
		@remarks
			Releases a specified sounds source object back to the system,
			allowing it to be re-used by another sound.
			@param
				sound Sound pointer.
		 */
		bool releaseSoundSource(OgreOggISound* sound=0);
		/** Sets distance model.
		@remarks
			Sets the global distance attenuation algorithm used by all
			sounds in the system.
			@param
				value ALenum value of distance model.
		 */
		void setDistanceModel(ALenum value);
		/** Sets doppler factor.
		@remarks
			Sets the global doppler factor which affects attenuation for
			all sounds
			@param
				factor Factor scale (>0).
		 */
		void setDopplerFactor(Ogre::Real factor=1.f);
		/** Sets speed of sound.
		@remarks
			Sets the global speed of sound used in the attenuation algorithm,
			affects all sounds.
			@param
				speed Speed (m/s).
		 */
		void setSpeedOfSound(Ogre::Real speed=363.f);
		/** Gets a list of device strings
		@remarks
			Creates a list of available audio device strings
		 */
		Ogre::StringVector getDeviceList();
		/** Returns pointer to listener.
		 */
		OgreOggListener *getListener(){return mListener;};
		/** Returns number of sources created.
		 */
		int getNumSources() { return mNumSources; }
		/** Opens all queued sounds.
		@remarks
			To prevent calling thread blocking when using ov_open_callbacks() we offload this call
			onto the update thread. This function basically opens an ogg file for reading and notifys
			its parent sound that its ready to be used.
		 */
		void processQueuedSounds(void);
		/** Updates system.
		@remarks
			Iterates all sounds and updates them.
			@param
				fTime Elapsed frametime.
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
			@param
				unit units(meters).
		 */
		void setEFXDistanceUnits(Ogre::Real unit=3.3f);
		/** Creates a specified EFX filter
		@remarks
			Creates a specified EFX filter if hardware supports it.
			@param
				eName name for filter.
			@param
				type see OpenAL docs for available filters.
		 */
		bool createEFXFilter(const std::string& eName, ALint type, ALfloat gain=1.0, ALfloat hfGain=1.0);
		/** Creates a specified EFX effect
		@remarks
			Creates a specified EFX effect if hardware supports it. Optional reverb
			preset structure can be passed which will be applied to the effect. See
			eax-util.h for list of presets.
			@param
				eName name for effect.
			@param
				type see OpenAL docs for available effects.
			@param
				props legacy structure describing a preset reverb effect.
		 */
		bool createEFXEffect(const std::string& eName, ALint type, EAXREVERBPROPERTIES* props=0);
		/** Sets extended properties on a specified sounds source
		@remarks
			Tries to set EFX extended source properties.
			@param
				sName name of sound.
			@param
				airAbsorption absorption factor for air.
			@param
				roomRolloff room rolloff factor.
			@param
				coneOuterHF cone outer gain factor for High frequencies.
		 */
		bool setEFXSoundProperties(const std::string& eName, Ogre::Real airAbsorption=0.f, Ogre::Real roomRolloff=0.f, Ogre::Real coneOuterHF=0.f);
		/** Sets a specified paremeter on an effect
		@remarks
			Tries to set a parameter value on a specified effect. Returns true/false.
			@param
				eName name of effect.
			@param
				effectType see OpenAL docs for available effects.
			@param
				attrib parameter value to alter.
			@param
				param float value to set.
		 */
		bool setEFXEffectParameter(const std::string& eName, ALint effectType, ALenum attrib, ALfloat param);
		/** Sets a specified paremeter on an effect
		@remarks
			Tries to set a parameter value on a specified effect. Returns true/false.
			@param
				eName name of effect.
			@param
				effectType see OpenAL docs for available effects.
			@param
				attrib parameter value to alter.
			@param
				param vector pointer of float values to set.
		 */
		bool setEFXEffectParameter(const std::string& eName, ALint type, ALenum attrib, ALfloat* params=0);
		/** Sets a specified paremeter on an effect
		@remarks
			Tries to set a parameter value on a specified effect. Returns true/false.
			@param
				eName name of effect.
			@param
				effectType see OpenAL docs for available effects.
			@param
				attrib parameter value to alter.
			@param
				param integer value to set.
		 */
		bool setEFXEffectParameter(const std::string& eName, ALint type, ALenum attrib, ALint param);
		/** Sets a specified paremeter on an effect
		@remarks
			Tries to set a parameter value on a specified effect. Returns true/false.
			@param
				eName name of effect.
			@param
				effectType see OpenAL docs for available effects.
			@param
				attrib parameter value to alter.
			@param
				params vector pointer of integer values to set.
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
			@param
				eName name for filter.
				type see OpenAL docs for available filter types.
		 */
		bool createEFXSlot();
		/** Attaches an effect to a sound
		@remarks
			Currently sound must have a source attached prior to this call.
			@param
				sName name of sound
			@param
				slot slot ID
			@param
				effect name of effect as defined when created
			@param
				filter name of filter as defined when created
		 */
		bool attachEffectToSound(const std::string& sName, ALuint slot, const Ogre::String& effect="", const Ogre::String& filter="");
		/** Attaches a filter to a sound
		@remarks
			Currently sound must have a source attached prior to this call.
			@param
				sName name of sound
			@param
				filter name of filter as defined when created
		 */
		bool attachFilterToSound(const std::string& sName, const Ogre::String& filter="");
		/** Detaches all effects from a sound
		@remarks
			Currently sound must have a source attached prior to this call.
			@param
				sName name of sound
			@param
				slot slot ID
		 */
		bool detachEffectFromSound(const std::string& sName, ALuint slotID);
		/** Detaches all filters from a sound
		@remarks
			Currently sound must have a source attached prior to this call.
			@param
				sName name of sound
		 */
		bool detachFilterFromSound(const std::string& sName);
		/** Returns whether a specified effect is supported
			@param
				effectID OpenAL effect/filter id. (AL_EFFECT... | AL_FILTER...)
		 */
		bool isEffectSupported(ALint effectID);
		/** Gets recording device
		 */
		OgreOggSoundRecord* getRecorder() { return mRecorder; }
		/** Returns whether a capture device is available
		 */
		bool isRecordingAvailable();
		/** Creates a recordable object
		 */
		OgreOggSoundRecord* createRecorder();
#endif
		/** Releases a shared audio buffer
		@remarks
			Each shared audio buffer is reference counted so destruction is handled correctly,
			this function merely decrements the reference count, only destroying when no sounds
			are referencing buffer.
			@param sName
				Name of audio file
		*/
		bool releaseSharedBuffer(const Ogre::String& sName, ALuint& buffer);
		/** Registers a shared audio buffer
		@remarks
			Its possible to share audio buffer data among many sources so this function
			registers an audio buffer as 'sharable', meaning if a the same audio file is
			created more then once, it will simply use the original buffer data instead of
			creating/loading the same data again.
			@param sName
				Name of audio file
			@param buffer
				OpenAL buffer ID holding audio data
		 */
		bool registerSharedBuffer(const Ogre::String& sName, ALuint& buffer);
		/** Queues a sound to play
		@remarks
			Synchronisation function for Threaded version of library, fixes the bug where creating 
			and immediately playing a sound through OgreOggISound::play() results in no sound. This 
			is because theres no facilty to re-check a play delayed sound after it has been opened.
			This function therefore adds the specified sound to an internal list which is checked 
			in update() and an attempt is made to play() the sound.
			NOTE (Multithreaded version ONLY):- calling OgreOggISound::play()/stop()/pause() directly 
			still has the potential to crash out as these functions are not mutex locked. If Threading
			is enabled you SHOULD use the OgreOggSoundManager::playSound()/pauseSound()/stopSound() functions.
			This will be addressed in the near future.
		 */
		void queueDelayedSound(OgreOggISound* sound=0, DELAYED_ACTION action=DA_PLAY);

#if OGGSOUND_THREADED

		boost::recursive_mutex mMutex;

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
				OgreOggSoundManager::getSingleton().processQueuedSounds();
				OgreOggSoundManager::getSingleton().updateBuffers();
				mUpdateThread->yield();
			}
		}

#endif
	protected:

		/** Destroys a single sound.
		@remarks
			Destroys a single sound object.
			@param
				sound
					Sound to destroy.
		 */
		void _destroySound(OgreOggISound* sound=0);
		/** Removes references of a sound from all possible internal lists.
		@remarks
			Various lists exist to manage numerous states of a sound, this
			function exists to remove a sound object from any/all lists it has
			previously been added to. 
			@param sound
				Sound to destroy.
		 */
		void _removeFromLists(OgreOggISound* sound=0);

	private:

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
		void _release();
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
		@param
			fName name of filter as defined when created.
		 */
		ALuint _getEFXFilter(const std::string& fName);
		/** Gets a specified EFX Effect
		@param
			eName name of effect as defined when created.
		 */
		ALuint _getEFXEffect(const std::string& eName);
		/** Gets a specified EFX Effect slot
		@param
			slotID index of auxiliary effect slot
		 */
		ALuint _getEFXSlot(int slotID=0);
		/** Sets EAX reverb properties using a specified present
		@param
			pEFXEAXReverb pointer to converted EFXEAXREVERBPROPERTIES structure object
		@param
			uiEffect effect ID
		 */
		bool _setEAXReverbProperties(EFXEAXREVERBPROPERTIES *pEFXEAXReverb, ALuint uiEffect);
		/** Attaches a created effect to an Auxiliary slot
		@param
			slot slot ID
		@param
			effect effect ID
		 */
		bool _attachEffectToSlot(ALuint slot, ALuint effect);
#endif
		/** Re-activates any sounds which had their source stolen.
		@remarks
			When all sources are in use the sounds begin to give up
			their source objects to higher priority sounds. When this
			happens the lower priority sound is queued ready to re-play
			when a source becomes available again, this function checks
			this queue and tries to re-play those sounds. Only affects
			sounds which were originally playing when forced to give up
			their source object.
		 */
		void _reactivateQueuedSounds();
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
		FileOpenList mQueuedSounds;				// list of sounds queued to be opened (multi-threaded ONLY)
		ActiveList mPlayQueue;					// list of sounds waiting to play	(only used prior to file loaded)
		ActiveList mPauseQueue;					// list of sounds waiting to pause	(only used prior to file loaded)
		ActiveList mStopQueue;					// list of sounds waiting to stop	(only used prior to file loaded)
		ActiveList mPausedSounds;				// list of sounds currently paused
		ActiveList mSoundsToReactivate;			// list of sounds that need re-activating when sources become available
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

		/** sort algorithms
		*/
		struct _sortNearToFar;
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
		std::vector<ALuint> mEffectSlotList;	// List of EFX effect slots

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
