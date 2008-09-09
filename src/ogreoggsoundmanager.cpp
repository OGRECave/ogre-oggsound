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

#include "OgreOggSoundManager.h"
#include <string>

#if OGGSOUND_THREADED
	boost::thread *OgreOggSound::OgreOggSoundManager::mUpdateThread = 0;
	bool OgreOggSound::OgreOggSoundManager::mShuttingDown = false;
#endif

template<> OgreOggSound::OgreOggSoundManager* Ogre::Singleton<OgreOggSound::OgreOggSoundManager>::ms_Singleton = 0;

namespace OgreOggSound
{
	using namespace Ogre;

	/*/////////////////////////////////////////////////////////////////*/
	OgreOggSoundManager::OgreOggSoundManager() : 
		mNumSources(0), 
		mDevice(0), 
		mContext(0), 
		mListener(0),
		mEAXSupport(false),
		mEFXSupport(false),
		mXRamSupport(false),
		mXRamSize(0),
		mXRamFree(0),
		mXRamAuto(0),
		mXRamHardware(0),
		mXRamAccessible(0),
		mCurrentXRamMode(0),
		mEAXVersion(0),
		mDeviceStrings(0)
	{
	}
	/*/////////////////////////////////////////////////////////////////*/
	OgreOggSoundManager::~OgreOggSoundManager()
	{
	#if OGGSOUND_THREADED
		mShuttingDown = true;
		if (mUpdateThread)
		{
			mUpdateThread->join();
			delete mUpdateThread;
			mUpdateThread = 0;
		}
#endif

		_release();			

		alcMakeContextCurrent(0);
		alcDestroyContext(mContext);
		alcCloseDevice(mDevice);
		
		delete mListener;
	}
	/*/////////////////////////////////////////////////////////////////*/
	OgreOggSoundManager* OgreOggSoundManager::getSingletonPtr(void)
	{
		return ms_Singleton;
	}
	/*/////////////////////////////////////////////////////////////////*/
	OgreOggSoundManager& OgreOggSoundManager::getSingleton(void)
	{  
		assert( ms_Singleton );  return ( *ms_Singleton );
	}
	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::init(const std::string &deviceName)
	{
		Ogre::LogManager::getSingleton().logMessage("***********************************");
		Ogre::LogManager::getSingleton().logMessage("*** --- Initialising OpenAL --- ***");
		Ogre::LogManager::getSingleton().logMessage("***********************************");

		// Get an internal list of audio device strings
		_enumDevices();

		int majorVersion;
		int minorVersion;

		// Version Info
		alcGetIntegerv(NULL, ALC_MAJOR_VERSION, sizeof(majorVersion), &majorVersion);
		if (alGetError()) throw std::string("Unable to get OpenAL Major Version number");
		alcGetIntegerv(NULL, ALC_MINOR_VERSION, sizeof(minorVersion), &minorVersion);
		if (alGetError()) throw std::string("Unable to get OpenAL Minor Version number");
		Ogre::String msg="*** --- OpenAL version " + Ogre::StringConverter::toString(majorVersion) + "." + Ogre::StringConverter::toString(minorVersion);
		Ogre::LogManager::getSingleton().logMessage(msg);

		/*
		** OpenAL versions prior to 1.0 DO NOT support device enumeration, so we
		** need to test the current version and decide if we should try to find 
		** an appropriate device or if we should just open the default device.
		*/
		bool deviceInList = false;
		if(majorVersion >= 1 && minorVersion >= 1)
		{
			Ogre::LogManager::getSingleton().logMessage("*** --- AVAILABLE DEVICES --- ***");

			// List devices in log and see if the sugested device is in the list
			Ogre::StringVector deviceList = getDeviceList();
			std::stringstream ss;
			Ogre::StringVector::iterator deviceItr;
			for(deviceItr = deviceList.begin(); deviceItr != deviceList.end() && (*deviceItr).compare("") != 0; deviceItr++)
			{
				deviceInList |= (*deviceItr).compare(deviceName) == 0;
				ss << "*** --- " << (*deviceItr);
				Ogre::LogManager::getSingleton().logMessage(ss.str());
				ss.clear(); ss.str("");
			}
		}

		// If the suggested device is in the list we use it, otherwise select the default device
		mDevice = alcOpenDevice(deviceInList ? deviceName.c_str() : NULL);
		if (!mDevice)
			throw std::string("Unable to create OpenAL device");

		if (!deviceInList)
			Ogre::LogManager::getSingleton().logMessage("*** --- Choosing: " + Ogre::String(alcGetString(mDevice, ALC_DEVICE_SPECIFIER))+" (Default device)");
		else
			Ogre::LogManager::getSingleton().logMessage("*** --- Choosing: " + Ogre::String(alcGetString(mDevice, ALC_DEVICE_SPECIFIER)));

		Ogre::LogManager::getSingleton().logMessage("*** --- OpenAL Device successfully created");
	
		mContext = alcCreateContext(mDevice,0);
		if (!mContext)
			throw std::string("Unable to create OpenAL context");

		Ogre::LogManager::getSingleton().logMessage("*** --- OpenAL Context successfully created");

		if (!alcMakeContextCurrent(mContext))
			throw std::string("Unable to set current OpenAL context");

		_checkFeatureSupport();

		mListener = new OgreOggListener;

		mNumSources = createSourcePool();

		msg="*** --- Created " + Ogre::StringConverter::toString(mNumSources) + " sources for simultaneous sounds";
		Ogre::LogManager::getSingleton().logMessage(msg);

	#if OGGSOUND_THREADED
		mUpdateThread = new boost::thread(boost::function0<void>(&OgreOggSoundManager::threadUpdate));
		Ogre::LogManager::getSingleton().logMessage("*** --- Using BOOST threads for streaming");
	#endif

		Ogre::LogManager::getSingleton().logMessage("***********************************");
		Ogre::LogManager::getSingleton().logMessage("*** ---  OpenAL Initialised --- ***");
		Ogre::LogManager::getSingleton().logMessage("***********************************");

		return true;
	}

	/*/////////////////////////////////////////////////////////////////*/
	Ogre::StringVector OgreOggSoundManager::getDeviceList()
	{
		const ALCchar* deviceList = alcGetString(NULL, ALC_DEVICE_SPECIFIER);

		Ogre::StringVector deviceVector;
		/*
		** The list returned by the call to alcGetString has the names of the
		** devices seperated by NULL characters and the list is terminated by
		** two NULL characters, so we can cast the list into a string and it
		** will automatically stop at the first NULL that it sees, then we
		** can move the pointer ahead by the lenght of that string + 1 and we
		** will be at the begining of the next string.  Once we hit an empty 
		** string we know that we've found the double NULL that terminates the
		** list and we can stop there.
		*/
		while(*deviceList != NULL)
		{
			try
			{
				ALCdevice *device = alcOpenDevice(deviceList);
				if (alcGetError(device)) throw std::string("Unable to open device");

				if(device)
				{
					// Device seems to be valid
					ALCcontext *context = alcCreateContext(device, NULL);
					if (alcGetError(device)) throw std::string("Unable to create context");
					if(context)
					{
						// Context seems to be valid
						alcMakeContextCurrent(context);
						if(alcGetError(device)) throw std::string("Unable to make context current");
						deviceVector.push_back(alcGetString(device, ALC_DEVICE_SPECIFIER));
						alcMakeContextCurrent(NULL);
						if(alcGetError(device)) throw std::string("Unable to clear current context");
						alcDestroyContext(context);
						if(alcGetError(device)) throw std::string("Unable to destroy current context");
					}
					alcCloseDevice(device);
				}
			}
			catch(...)
			{
				// Don't die here, we'll just skip this device.
			}

			deviceList += strlen(deviceList) + 1;
		}

		return deviceVector;
	}
	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::_checkEFXSupport()
	{
		if (alcIsExtensionPresent(mDevice, "ALC_EXT_EFX"))
		{
			/* Get function pointers
			alGenEffects = (LPALGENEFFECTS)alGetProcAddress("alGenEffects");
			alDeleteEffects = (LPALDELETEEFFECTS )alGetProcAddress("alDeleteEffects");
			alIsEffect = (LPALISEFFECT )alGetProcAddress("alIsEffect");
			alEffecti = (LPALEFFECTI)alGetProcAddress("alEffecti");
			alEffectiv = (LPALEFFECTIV)alGetProcAddress("alEffectiv");
			alEffectf = (LPALEFFECTF)alGetProcAddress("alEffectf");
			alEffectfv = (LPALEFFECTFV)alGetProcAddress("alEffectfv");
			alGetEffecti = (LPALGETEFFECTI)alGetProcAddress("alGetEffecti");
			alGetEffectiv = (LPALGETEFFECTIV)alGetProcAddress("alGetEffectiv");
			alGetEffectf = (LPALGETEFFECTF)alGetProcAddress("alGetEffectf");
			alGetEffectfv = (LPALGETEFFECTFV)alGetProcAddress("alGetEffectfv");
			alGenFilters = (LPALGENFILTERS)alGetProcAddress("alGenFilters");
			alDeleteFilters = (LPALDELETEFILTERS)alGetProcAddress("alDeleteFilters");
			alIsFilter = (LPALISFILTER)alGetProcAddress("alIsFilter");
			alFilteri = (LPALFILTERI)alGetProcAddress("alFilteri");
			alFilteriv = (LPALFILTERIV)alGetProcAddress("alFilteriv");
			alFilterf = (LPALFILTERF)alGetProcAddress("alFilterf");
			alFilterfv = (LPALFILTERFV)alGetProcAddress("alFilterfv");
			alGetFilteri = (LPALGETFILTERI )alGetProcAddress("alGetFilteri");
			alGetFilteriv= (LPALGETFILTERIV )alGetProcAddress("alGetFilteriv");
			alGetFilterf = (LPALGETFILTERF )alGetProcAddress("alGetFilterf");
			alGetFilterfv= (LPALGETFILTERFV )alGetProcAddress("alGetFilterfv");
			alGenAuxiliaryEffectSlots = (LPALGENAUXILIARYEFFECTSLOTS)alGetProcAddress("alGenAuxiliaryEffectSlots");
			alDeleteAuxiliaryEffectSlots = (LPALDELETEAUXILIARYEFFECTSLOTS)alGetProcAddress("alDeleteAuxiliaryEffectSlots");
			alIsAuxiliaryEffectSlot = (LPALISAUXILIARYEFFECTSLOT)alGetProcAddress("alIsAuxiliaryEffectSlot");
			alAuxiliaryEffectSloti = (LPALAUXILIARYEFFECTSLOTI)alGetProcAddress("alAuxiliaryEffectSloti");
			alAuxiliaryEffectSlotiv = (LPALAUXILIARYEFFECTSLOTIV)alGetProcAddress("alAuxiliaryEffectSlotiv");
			alAuxiliaryEffectSlotf = (LPALAUXILIARYEFFECTSLOTF)alGetProcAddress("alAuxiliaryEffectSlotf");
			alAuxiliaryEffectSlotfv = (LPALAUXILIARYEFFECTSLOTFV)alGetProcAddress("alAuxiliaryEffectSlotfv");
			alGetAuxiliaryEffectSloti = (LPALGETAUXILIARYEFFECTSLOTI)alGetProcAddress("alGetAuxiliaryEffectSloti");
			alGetAuxiliaryEffectSlotiv = (LPALGETAUXILIARYEFFECTSLOTIV)alGetProcAddress("alGetAuxiliaryEffectSlotiv");
			alGetAuxiliaryEffectSlotf = (LPALGETAUXILIARYEFFECTSLOTF)alGetProcAddress("alGetAuxiliaryEffectSlotf");
			alGetAuxiliaryEffectSlotfv = (LPALGETAUXILIARYEFFECTSLOTFV)alGetProcAddress("alGetAuxiliaryEffectSlotfv");

			if (alGenEffects &&	alDeleteEffects && alIsEffect && alEffecti && alEffectiv &&	alEffectf &&
				alEffectfv && alGetEffecti && alGetEffectiv && alGetEffectf && alGetEffectfv &&	alGenFilters &&
				alDeleteFilters && alIsFilter && alFilteri && alFilteriv &&	alFilterf && alFilterfv &&
				alGetFilteri &&	alGetFilteriv && alGetFilterf && alGetFilterfv && alGenAuxiliaryEffectSlots &&
				alDeleteAuxiliaryEffectSlots &&	alIsAuxiliaryEffectSlot && alAuxiliaryEffectSloti &&
				alAuxiliaryEffectSlotiv && alAuxiliaryEffectSlotf && alAuxiliaryEffectSlotfv &&
				alGetAuxiliaryEffectSloti && alGetAuxiliaryEffectSlotiv && alGetAuxiliaryEffectSlotf &&
				alGetAuxiliaryEffectSlotfv)
				return true;*/
		}

		return false;
	}

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::_checkXRAMSupport()
	{
		// Check for X-RAM extension
		if(alIsExtensionPresent("EAX-RAM") == AL_TRUE)
		{
			// Get X-RAM Function pointers
			mEAXSetBufferMode = (EAXSetBufferMode)alGetProcAddress("EAXSetBufferMode");
			mEAXGetBufferMode = (EAXGetBufferMode)alGetProcAddress("EAXGetBufferMode");

			if (mEAXSetBufferMode && mEAXGetBufferMode)
			{
				mXRamSize = alGetEnumValue("AL_EAX_RAM_SIZE");
				mXRamFree = alGetEnumValue("AL_EAX_RAM_FREE");
				mXRamAuto = alGetEnumValue("AL_STORAGE_AUTOMATIC");
				mXRamHardware = alGetEnumValue("AL_STORAGE_HARDWARE");
				mXRamAccessible = alGetEnumValue("AL_STORAGE_ACCESSIBLE");

				if (mXRamSize && mXRamFree && mXRamAuto && mXRamHardware && mXRamAccessible)
				{
					// Support available
					mXRamSizeMB = alGetInteger(mXRamSize) / (1024*1024);
					mXRamFreeMB = alGetInteger(mXRamFree) / (1024*1024);
					return true;
				}
			}
		}	
		return false;
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::_checkFeatureSupport()
	{
		Ogre::String msg="";
		// Supported Formats Info
		Ogre::LogManager::getSingleton().logMessage("*** --- SUPPORTED FORMATS");
		ALenum eBufferFormat = 0;
		eBufferFormat = alcGetEnumValue(mDevice, "AL_FORMAT_MONO16");
		if(eBufferFormat) 
		{
			msg="*** --- AL_FORMAT_MONO16 -- Monophonic Sound";
			Ogre::LogManager::getSingleton().logMessage(msg);
		}
		eBufferFormat = alcGetEnumValue(mDevice, "AL_FORMAT_STEREO16");
		if(eBufferFormat) 
		{
			msg="*** --- AL_FORMAT_STEREO16 -- Stereo Sound";
			Ogre::LogManager::getSingleton().logMessage(msg);
		}
		eBufferFormat = alcGetEnumValue(mDevice, "AL_FORMAT_QUAD16");
		if(eBufferFormat) 
		{
			msg="*** --- AL_FORMAT_QUAD16 -- 4 Channel Sound";
			Ogre::LogManager::getSingleton().logMessage(msg);
		}
		eBufferFormat = alcGetEnumValue(mDevice, "AL_FORMAT_51CHN16");
		if(eBufferFormat) 
		{
			msg="*** --- AL_FORMAT_51CHN16 -- 5.1 Surround Sound";
			Ogre::LogManager::getSingleton().logMessage(msg);
		}
		eBufferFormat = alcGetEnumValue(mDevice, "AL_FORMAT_61CHN16");
		if(eBufferFormat) 
		{
			msg="*** --- AL_FORMAT_61CHN16 -- 6.1 Surround Sound";
			Ogre::LogManager::getSingleton().logMessage(msg);
		}
		eBufferFormat = alcGetEnumValue(mDevice, "AL_FORMAT_71CHN16");
		if(eBufferFormat) 
		{
			msg="*** --- AL_FORMAT_71CHN16 -- 7.1 Surround Sound";
			Ogre::LogManager::getSingleton().logMessage(msg);
		}

		// EFX
		mEFXSupport = _checkEFXSupport();
		if (mEFXSupport)
		{
			Ogre::LogManager::getSingleton().logMessage("*** --- EFX Detected");
		}
		else
			Ogre::LogManager::getSingleton().logMessage("*** --- EFX NOT Detected");

		// XRAM
		mXRamSupport = _checkXRAMSupport();
		if (mXRamSupport)
		{
			// Log message
			Ogre::LogManager::getSingleton().logMessage("*** --- X-RAM Detected");
			Ogre::LogManager::getSingleton().logMessage("*** --- X-RAM Size(MB): " + Ogre::StringConverter::toString(mXRamSizeMB) +
				" Free(MB):" + Ogre::StringConverter::toString(mXRamFreeMB));		
		}
		else
			Ogre::LogManager::getSingleton().logMessage("*** --- XRAM NOT Detected");

		// EAX 
		for(int version = 5; version >= 2; version--)
		{
			Ogre::String eaxName="EAX"+Ogre::StringConverter::toString(version)+".0";
			if(alIsExtensionPresent(eaxName.c_str()) == AL_TRUE)
			{
				mEAXSupport = true;
				mEAXVersion = version;
				eaxName="*** --- EAX "+Ogre::StringConverter::toString(version)+".0 Detected";
				Ogre::LogManager::getSingleton().logMessage(eaxName);
				break;
			}
		}
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::_enumDevices()
	{
		mDeviceStrings = const_cast<ALCchar*>(alcGetString(0,ALC_DEVICE_SPECIFIER));
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::setXRamBuffer(ALsizei numBuffers, ALuint* buffer)
	{
		if ( buffer && mEAXSetBufferMode )
			mEAXSetBufferMode(numBuffers, buffer, mCurrentXRamMode);
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::setXRamBufferMode(ALenum mode)
	{
		mCurrentXRamMode = mXRamAuto;
		if		( mode==mXRamAuto ) mCurrentXRamMode = mXRamAuto;
		else if ( mode==mXRamHardware ) mCurrentXRamMode = mXRamHardware;
		else if ( mode==mXRamAccessible ) mCurrentXRamMode = mXRamAccessible;
	}
	/*/////////////////////////////////////////////////////////////////*/
	OgreOggISound* OgreOggSoundManager::createSound(const std::string& name,const std::string& file, bool stream, bool loop)
	{
	#if OGGSOUND_THREADED
		boost::recursive_mutex::scoped_lock l(mMutex);
	#endif

		Ogre::ResourceGroupManager *groupManager = Ogre::ResourceGroupManager::getSingletonPtr();
		Ogre::String group = groupManager->findGroupContainingResource(file);
		Ogre::DataStreamPtr soundData = groupManager->openResource(file, group);

		if(file.find(".ogg") != std::string::npos || file.find(".OGG") != std::string::npos)
		{
			// MUST be unique
			if ( mSoundMap.find(name)!=mSoundMap.end() ) 
			{
				Ogre::String msg="*** OgreOggSoundManager::createSound() - Sound with name: "+name+" already exists!";
				Ogre::LogManager::getSingleton().logMessage(msg);
				return 0;
			}

			if(stream)
				mSoundMap[name] = new OgreOggStreamSound(name);
			else 
				mSoundMap[name] = new OgreOggStaticSound(name);

			// Read audio file
			mSoundMap[name]->open(soundData);
			// Set loop flag
			mSoundMap[name]->loop(loop);

			return mSoundMap[name];
		}
		else
		{
			Ogre::LogManager::getSingleton().logMessage("*** OgreOggSoundManager::createSound() - Sound name not ogg");
			return NULL;
		}
	}

	/*/////////////////////////////////////////////////////////////////*/
	OgreOggISound *OgreOggSoundManager::getSound(const std::string& name)
	{

	#if OGGSOUND_THREADED
		boost::recursive_mutex::scoped_lock l(mMutex);
	#endif

		SoundMap::iterator i = mSoundMap.find(name);
		if(i == mSoundMap.end()) return 0;
		return i->second;
	}
	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::hasSound(const std::string& name)
	{

	#if OGGSOUND_THREADED
		boost::recursive_mutex::scoped_lock l(mMutex);
	#endif

		SoundMap::iterator i = mSoundMap.find(name);
		if(i == mSoundMap.end()) return false; return true;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::setMasterVolume(ALfloat vol)
	{
		if ( mListener ) mListener->setListenerVolume(vol);
	}
	/*/////////////////////////////////////////////////////////////////*/
	ALfloat OgreOggSoundManager::getMasterVolume()
	{
		if (mListener) return mListener->getListenerVolume(); else return 1.0;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::destroyAllSounds()
	{
	#if OGGSOUND_THREADED
		boost::recursive_mutex::scoped_lock l(mMutex);
	#endif

		SoundMap::iterator i = mSoundMap.begin();
		while(i != mSoundMap.end())
		{
			delete i->second;
			++i;
		}

		mSoundMap.clear();

		SourceList::iterator it = mSourcePool.begin();
		while (it != mSourcePool.end())
		{
			++it;
		}
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::stopAllSounds()
	{
	#if OGGSOUND_THREADED
		boost::recursive_mutex::scoped_lock l(mMutex);
	#endif

		if (mActiveSounds.empty()) return;

		for (ActiveList::iterator iter=mActiveSounds.begin(); iter!=mActiveSounds.end(); ++iter)
		{
			(*iter)->stop();
		}
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::pauseAllSounds()
	{
	#if OGGSOUND_THREADED
		boost::recursive_mutex::scoped_lock l(mMutex);
	#endif

		if (mActiveSounds.empty()) return;

		for (ActiveList::iterator iter=mActiveSounds.begin(); iter!=mActiveSounds.end(); ++iter)
		{
			if ( (*iter)->isPlaying() )
				mPausedSounds.push_back((*iter));
		}
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::resumeAllPausedSounds()
	{
	#if OGGSOUND_THREADED
		boost::recursive_mutex::scoped_lock l(mMutex);
	#endif

		if (mPausedSounds.empty()) return;

		for (ActiveList::iterator iter=mPausedSounds.begin(); iter!=mPausedSounds.end(); ++iter)
			(*iter)->play();

		mPausedSounds.clear();
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::destroySound(const Ogre::String& sName)
	{
	#if OGGSOUND_THREADED
		boost::recursive_mutex::scoped_lock l(mMutex);
	#endif

		if ( sName.empty() ) return;

		SoundMap::iterator i = mSoundMap.find(sName);
		if (i != mSoundMap.end())
		{
			// Remove from active sounds list
			if ( !mActiveSounds.empty() )
			{
				for ( ActiveList::iterator iter=mActiveSounds.begin(); iter!=mActiveSounds.end(); ++iter )
					if ( i->second==(*iter) )
					{
						mActiveSounds.erase(iter);
						break;
					}
			}

			// Remove from reactivate list
			if ( !mSoundsToReactivate.empty() )
			{
				for ( ActiveList::iterator iter=mSoundsToReactivate.begin(); iter!=mSoundsToReactivate.end(); ++iter )
					if ( i->second==(*iter) )
					{
						mSoundsToReactivate.erase(iter);
						break;
					}
			}

			// Delete sound
			delete i->second;
			// Remove from main list
			mSoundMap.erase(i);
		}

	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::updateBuffers()
	{
		ActiveList::iterator i = mActiveSounds.begin();
		while( i != mActiveSounds.end())
		{
		#if OGGSOUND_THREADED
			boost::recursive_mutex::scoped_lock l(mMutex);
		#endif
			(*i)->updateAudioBuffers();
			++i;
		}	

	#if (OGGSOUND_THREADED!=0)
		Sleep(10);	
	#endif
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::_release()
	{

	#if OGGSOUND_THREADED
		boost::recursive_mutex::scoped_lock l(mMutex);
	#endif

		SoundMap::iterator i = mSoundMap.begin();
		while(i != mSoundMap.end())
		{
			delete i->second;
			++i;
		}

		mSoundMap.clear();

		SourceList::iterator it = mSourcePool.begin();
		while (it != mSourcePool.end())
		{
			alDeleteSources(1,&(*it));
			++it;
		}

		mSourcePool.clear();
	}
	/*/////////////////////////////////////////////////////////////////*/
	int OgreOggSoundManager::createSourcePool()
	{
		ALuint source;
		int numSources = 0;

		while(alGetError() == AL_NO_ERROR && numSources < MAX_SOURCES)
		{
			source = 0;
			alGenSources(1,&source);
			if(source != 0)
			{
				mSourcePool.push_back(source);
				numSources++;
			}
			else
			{
				alGetError();
				break;
			}
		}

		return static_cast<int>(mSourcePool.size());
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::_reactivateQueuedSounds()
	{
	#if OGGSOUND_THREADED
		boost::recursive_mutex::scoped_lock l(mMutex);
	#endif

		// Any sounds to re-activate?
		if (mSoundsToReactivate.empty()) return;

		ActiveList::iterator iter=mSoundsToReactivate.begin(); 

		// Loop through all queued sounds
		while ( iter!=mSoundsToReactivate.end() )
		{
			bool needIncrement=true;

			// If there are sources available
			// Use immediately
			if ( !mSourcePool.empty() ) 
			{
				// Get next available source
				ALuint src = static_cast<ALuint>(mSourcePool.back());

				// Remove from available list
				mSourcePool.pop_back();

				// Set sounds source
				(*iter)->setSource(src);

				// Add to active list
				mActiveSounds.push_back((*iter));

				// Remove from queued list
				iter = mSoundsToReactivate.erase(iter);

				// Cancel increment
				needIncrement=false;
			}
			// All sources in use 
			// Re-use an active source
			// Use either a non-playing source or a lower priority source
			else
			{
				// Search for a stopped active sound
				for ( ActiveList::iterator aIter=mActiveSounds.begin(); aIter!=mActiveSounds.end(); ++aIter )
				{
					// Find a stopped sound - reuse its source
					if ( (*aIter)->isStopped() )
					{
						// Release sounds source
						if (releaseSoundSource((*aIter)))
						{
							// Request new source for reactivated sound
							(*iter)->play();
							
							// sound playing - remove from reactivate list 
							iter = mSoundsToReactivate.erase(iter);				

							// Cancel increment
							needIncrement=false;
						}
					}
				}
				// Increment to next reactivated sound
				if (needIncrement) ++iter;
			}
		}
	}
	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::requestSoundSource(OgreOggISound* sound)
	{
	#if OGGSOUND_THREADED
		boost::recursive_mutex::scoped_lock l(mMutex);
	#endif

		// Does sound need a source?
		if (!sound || sound->getSource()!=AL_NONE) return false;

		ALuint src = AL_NONE;

		// If there are still sources available
		// Pop next available off list
		if ( !mSourcePool.empty() ) 
		{
			// Get next available source
			src = static_cast<ALuint>(mSourcePool.back());
			// Remove from available list
			mSourcePool.pop_back();
			// log message
			// Set sounds source
			sound->setSource(src);
			// Add to active list
			mActiveSounds.push_back(sound);
			return true;
		}
		// All sources in use 
		// Re-use an active source
		// Use either a non-playing source or a lower priority source
		else
		{
			// Search for a stopped sound
			for ( ActiveList::iterator iter=mActiveSounds.begin(); iter!=mActiveSounds.end(); ++iter )
			{
				// Find a stopped sound - reuse its source
				if ( (*iter)->isStopped() )
				{
					// Release lower priority sounds source
					if (releaseSoundSource((*iter))) 
						return requestSoundSource(sound);
				}
			}

			// Check priority...
			Ogre::uint8 priority = sound->getPriority();
			for ( ActiveList::iterator iter=mActiveSounds.begin(); iter!=mActiveSounds.end(); ++iter )
			{
				// Find a similar/lesser prioritised sound and re-use source
				if ( (*iter)->getPriority() <= priority )
				{
					// Queue sound to re-activate itself when possible
					mSoundsToReactivate.push_back((*iter));

					// Release lower priority sounds source
					if (releaseSoundSource((*iter))) 
						return requestSoundSource(sound);
				}
			}
		}
		// Uh oh - won't be played
		return false;
	}

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::releaseSoundSource(OgreOggISound* sound)
	{

	#if OGGSOUND_THREADED
		boost::recursive_mutex::scoped_lock l(mMutex);
	#endif

		if (!sound || sound->getSource()==AL_NONE)
		{
			return false;
		}
	
		// Get source
		ALuint src = sound->getSource();

		// Valid source?
		if(src!=AL_NONE)
		{
			ALuint source=AL_NONE;
	
			// Detach source from sound
			sound->setSource(source);

			// Remove from actives list
			for ( ActiveList::iterator iter=mActiveSounds.begin(); iter!=mActiveSounds.end(); ++iter )
			{
				// Find sound in actives list
				if ( (*iter)==sound )	
				{
					// Remove from list
					mActiveSounds.erase(iter);

					// Make source available
					mSourcePool.push_back(src);

					// All ok
					return true;
				}
			}
		}

		return false;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::setDistanceModel(ALenum value)
	{
		alDistanceModel(value);
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::setSpeedOfSound(Ogre::Real speed)
	{
		alSpeedOfSound(speed);
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::setDopplerFactor(Ogre::Real factor)
	{
		alDopplerFactor(factor);
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::update(Ogre::Real fTime)
	{		
		ActiveList::iterator i = mActiveSounds.begin();
		while( i != mActiveSounds.end())
		{
			(*i)->update(fTime);
	#if ( OGGSOUND_THREADED==0 )
			(*i)->updateAudioBuffers();
	#endif
			i++;
		}

		_reactivateQueuedSounds();

		mListener->update();
	}
}