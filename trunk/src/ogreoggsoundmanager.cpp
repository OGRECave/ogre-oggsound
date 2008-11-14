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
		mFilterList(0),
		mEffectList(0),
		mEffectSlotList(0),
		mDeviceStrings(0)
		{
			// Effect objects
			alGenEffects = NULL;
			alDeleteEffects = NULL;
			alIsEffect = NULL;
			alEffecti = NULL;
			alEffectiv = NULL;
			alEffectf = NULL;
			alEffectfv = NULL;
			alGetEffecti = NULL;
			alGetEffectiv = NULL;
			alGetEffectf = NULL;
			alGetEffectfv = NULL;

			//Filter objects
			alGenFilters = NULL;
			alDeleteFilters = NULL;
			alIsFilter = NULL;
			alFilteri = NULL;
			alFilteriv = NULL;
			alFilterf = NULL;
			alFilterfv = NULL;
			alGetFilteri = NULL;
			alGetFilteriv = NULL;
			alGetFilterf = NULL;
			alGetFilterfv = NULL;

			// Auxiliary slot object
			alGenAuxiliaryEffectSlots = NULL;
			alDeleteAuxiliaryEffectSlots = NULL;
			alIsAuxiliaryEffectSlot = NULL;
			alAuxiliaryEffectSloti = NULL;
			alAuxiliaryEffectSlotiv = NULL;
			alAuxiliaryEffectSlotf = NULL;
			alAuxiliaryEffectSlotfv = NULL;
			alGetAuxiliaryEffectSloti = NULL;
			alGetAuxiliaryEffectSlotiv = NULL;
			alGetAuxiliaryEffectSlotf = NULL;
			alGetAuxiliaryEffectSlotfv = NULL;
			
			mNumEffectSlots = 0;
			mNumSendsPerSource = 0;
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
		if (!ms_Singleton) ms_Singleton = new OgreOggSoundManager();
		return ms_Singleton;
	}
	/*/////////////////////////////////////////////////////////////////*/
	OgreOggSoundManager& OgreOggSoundManager::getSingleton(void)
	{  
		if (!ms_Singleton) ms_Singleton = new OgreOggSoundManager();
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
	
		ALint attribs[2] = {ALC_MAX_AUXILIARY_SENDS, 4};

		mContext = alcCreateContext(mDevice, attribs);
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
	bool OgreOggSoundManager::setEFXEffectParameter(const std::string& eName, ALint effectType, ALenum attrib, ALfloat param)
	{
		if ( !hasEFXSupport() && eName.empty() ) return false;

		ALuint effect;

		// Get effect id's
		if ( (effect = _getEFXEffect(eName) ) != AL_EFFECT_NULL )
		{
			alGetError();
			alEffecti(effectType, attrib, param);
			if ( alGetError()!=AL_NO_ERROR )
			{
				Ogre::LogManager::getSingleton().logMessage("*** OgreOggSoundManager::setEFXEffectParameter() - Unable to change effect parameter!");
				return false;
			}
		}

		return false;
	}

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::setEFXEffectParameter(const std::string& eName, ALint effectType, ALenum attrib, ALfloat* params)
	{
		if ( !hasEFXSupport() && eName.empty() || !params ) return false;

		ALuint effect;

		// Get effect id's
		if ( (effect = _getEFXEffect(eName) ) != AL_EFFECT_NULL )
		{
			alGetError();
			alEffectfv(effectType, attrib, params);
			if ( alGetError()!=AL_NO_ERROR )
			{
				Ogre::LogManager::getSingleton().logMessage("*** OgreOggSoundManager::setEFXEffectParameter() - Unable to change effect parameters!");
				return false;
			}
		}

		return false;
	}

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::setEFXEffectParameter(const std::string& eName, ALint effectType, ALenum attrib, ALint param)
	{
		if ( !hasEFXSupport() && eName.empty() ) return false;

		ALuint effect;

		// Get effect id's
		if ( (effect = _getEFXEffect(eName) ) != AL_EFFECT_NULL )
		{
			alGetError();
			alEffecti(effectType, attrib, param);
			if ( alGetError()!=AL_NO_ERROR )
			{
				Ogre::LogManager::getSingleton().logMessage("*** OgreOggSoundManager::setEFXEffectParameter() - Unable to change effect parameter!");
				return false;
			}
		}

		return false;
	}

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::setEFXEffectParameter(const std::string& eName, ALint effectType, ALenum attrib, ALint* params)
	{
		if ( !hasEFXSupport() && eName.empty() || !params ) return false;

		ALuint effect;

		// Get effect id's
		if ( (effect = _getEFXEffect(eName) ) != AL_EFFECT_NULL )
		{
			alGetError();
			alEffectiv(effectType, attrib, params);
			if ( alGetError()!=AL_NO_ERROR )
			{
				Ogre::LogManager::getSingleton().logMessage("*** OgreOggSoundManager::setEFXEffectParameter() - Unable to change effect parameters!");
				return false;
			}
		}

		return false;
	}

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::attachEffectToSound(const std::string& sName, ALuint slotID, const Ogre::String& effectName, const Ogre::String& filterName)
	{

	#if OGGSOUND_THREADED
		boost::recursive_mutex::scoped_lock l(mMutex);
	#endif

		if ( !hasEFXSupport() && sName.empty() ) return false;

		ALuint effect;
		ALuint filter;
		ALuint slot;

		// Get effect id's
		slot	= _getEFXSlot(slotID);
		effect	= _getEFXEffect(effectName);
		filter	= _getEFXFilter(filterName);

		// Attach effect and filter to slot
		if ( _attachEffectToSlot(slot, effect) )
		{
			OgreOggISound* sound = getSound(sName);

			if ( sound )
			{
				ALuint src = sound->getSource();
				if ( src!=AL_NONE )
				{
					alSource3i(src, AL_AUXILIARY_SEND_FILTER, effect, slotID, filter);
					if (alGetError() == AL_NO_ERROR)
					{
						return true;
					}
					else
					{
						Ogre::LogManager::getSingleton().logMessage("*** OgreOggSoundManager::attachEffectToSound() - Unable to attach effect to source!");
						return false;
					}
				}
				else
				{
					Ogre::LogManager::getSingleton().logMessage("*** OgreOggSoundManager::attachEffectToSound() - sound has no source!");
					return false;
				}
			}
			else
			{
				Ogre::LogManager::getSingleton().logMessage("*** OgreOggSoundManager::attachEffectToSound() - sound not found!");
				return false;
			}
		}
		return false;
	}

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::attachFilterToSound(const std::string& sName, const Ogre::String& filterName)
	{
	#if OGGSOUND_THREADED
		boost::recursive_mutex::scoped_lock l(mMutex);
	#endif

		if ( !hasEFXSupport() && sName.empty() ) return false;

		ALuint filter = _getEFXFilter(filterName);

		if ( filter!=AL_FILTER_NULL )
		{
			OgreOggISound* sound = getSound(sName);

			if ( sound )
			{
				ALuint src = sound->getSource();
				if ( src!=AL_NONE )
				{
					alSourcei(src, AL_DIRECT_FILTER, filter);
					if (alGetError() == AL_NO_ERROR)
					{
						return true;
					}
					else
					{
						Ogre::LogManager::getSingleton().logMessage("*** OgreOggSoundManager::attachFilterToSound() - Unable to attach filter to source!");
						return false;
					}
				}
				else
				{
					Ogre::LogManager::getSingleton().logMessage("*** OgreOggSoundManager::attachFilterToSound() - sound has no source!");
					return false;
				}
			}
			else
			{
				Ogre::LogManager::getSingleton().logMessage("*** OgreOggSoundManager::attachFilterToSound() - sound not found!");
				return false;
			}

		}
		return false;
	}

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::detachEffectFromSound(const std::string& sName, ALuint slotID)
	{
	#if OGGSOUND_THREADED
		boost::recursive_mutex::scoped_lock l(mMutex);
	#endif

		if ( !hasEFXSupport() && sName.empty() ) return false;

		ALuint slot;

		// Get slot
		slot = _getEFXSlot(slotID);

		// Detach effect from sound
		if ( slot!=AL_NONE )
		{
			OgreOggISound* sound = getSound(sName);

			if ( sound )
			{
				ALuint src = sound->getSource();
				if ( src!=AL_NONE )
				{
					alSource3i(src, AL_AUXILIARY_SEND_FILTER, AL_EFFECT_NULL, slot, AL_FILTER_NULL);
					if (alGetError() == AL_NO_ERROR)
					{
						return true;
					}
					else
					{
						Ogre::LogManager::getSingleton().logMessage("*** OgreOggSoundManager::detachEffectFromSound() - Unable to detach effect from source!");
						return false;
					}
				}
				else
				{
					Ogre::LogManager::getSingleton().logMessage("*** OgreOggSoundManager::detachEffectFromSound() - sound has no source!");
					return false;
				}
			}
			else
			{
				Ogre::LogManager::getSingleton().logMessage("*** OgreOggSoundManager::detachEffectFromSound() - sound not found!");
				return false;
			}
		}
		return false;
	}

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::detachFilterFromSound(const std::string& sName)
	{
	#if OGGSOUND_THREADED
		boost::recursive_mutex::scoped_lock l(mMutex);
	#endif

		if ( !hasEFXSupport() && sName.empty() ) return false;

		OgreOggISound* sound = getSound(sName);

		if ( sound )
		{
			ALuint src = sound->getSource();
			if ( src!=AL_NONE )
			{
				alSourcei(src, AL_DIRECT_FILTER, AL_FILTER_NULL);
				if (alGetError() != AL_NO_ERROR)
				{
					Ogre::LogManager::getSingleton().logMessage("*** OgreOggSoundManager::dettachFilterToSound() - Unable to detach filter from source!");
					return false;
				}
			}
			else
			{
				Ogre::LogManager::getSingleton().logMessage("*** OgreOggSoundManager::detachFilterFromSound() - sound has no source!");
				return false;
			}
		}
		else
		{
			Ogre::LogManager::getSingleton().logMessage("*** OgreOggSoundManager::detachFilterFromSound() - sound not found!");
			return false;
		}

		return true;
	}

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::_attachEffectToSlot(ALuint slot, ALuint effect)
	{
		if ( !hasEFXSupport() || slot==AL_NONE ) return false;

		/* Attach Effect to Auxiliary Effect Slot */
		/* slot is the ID of an Aux Effect Slot */
		/* effect is the ID of an Effect */
		alAuxiliaryEffectSloti(slot, AL_EFFECTSLOT_EFFECT, effect);
		if (alGetError() != AL_NO_ERROR)
		{
			Ogre::LogManager::getSingleton().logMessage("*** Cannot attach effect to slot!");
			return false;
		}
		return true;
	}	
	/*/////////////////////////////////////////////////////////////////*/
	ALuint OgreOggSoundManager::_getEFXFilter(const std::string& fName)
	{
		if ( !mFilterList || !hasEFXSupport() || fName.empty() ) return AL_FILTER_NULL;

		EffectList::iterator filter=mFilterList->find(fName);
		if ( filter==mFilterList->end() ) 
			return AL_FILTER_NULL; 
		else 
			return filter->second;
	}

	/*/////////////////////////////////////////////////////////////////*/
	ALuint OgreOggSoundManager::_getEFXEffect(const std::string& eName)
	{
		if ( !mEffectList || !hasEFXSupport() || eName.empty() ) return AL_EFFECT_NULL;

		EffectList::iterator effect=mEffectList->find(eName);
		if ( effect==mEffectList->end() ) 
			return AL_EFFECT_NULL; 
		else 
			return effect->second;
	}

	/*/////////////////////////////////////////////////////////////////*/
	ALuint OgreOggSoundManager::_getEFXSlot(int slotID)
	{
		if ( !mEffectSlotList || !hasEFXSupport() || (slotID>=static_cast<int>(mEffectSlotList->size())) ) return AL_NONE;

		return static_cast<ALuint>((*mEffectSlotList)[slotID]);
	}

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::createEFXFilter(const std::string& fName, ALint filterType, ALfloat gain, ALfloat hfGain)
	{
		if ( !hasEFXSupport() || fName.empty() || !isEffectSupported(filterType) ) 
		{
			Ogre::LogManager::getSingleton().logMessage("*** OgreOggSoundManager::createEFXFilter() - Unsupported filter!");
			return false;
		}

		ALuint filter;

		alGenFilters(1, &filter);
		if (alGetError() != AL_NO_ERROR)
		{
			Ogre::LogManager::getSingleton().logMessage("*** Cannot create EFX Filter!");
			return false;
		}

		if (alIsFilter(filter) && ((filterType==AL_FILTER_LOWPASS) || (filterType==AL_FILTER_HIGHPASS) || (filterType==AL_FILTER_BANDPASS) ))
		{
			alFilteri(filter, AL_FILTER_TYPE, filterType);
			if (alGetError() != AL_NO_ERROR)
			{
				Ogre::LogManager::getSingleton().logMessage("*** Filter not supported!");
				return false;
			}
			else
			{
				// Set properties
				alFilterf(filter, AL_LOWPASS_GAIN, gain);
				alFilterf(filter, AL_LOWPASS_GAINHF, hfGain);

				if ( !mFilterList ) mFilterList = new EffectList;
				(*mFilterList)[fName]=filter;
			}
		}
		return true;
	}

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::createEFXEffect(const std::string& eName, ALint effectType, EAXREVERBPROPERTIES* props)
	{
		if ( !hasEFXSupport() || eName.empty() || !isEffectSupported(effectType) ) 		
		{
			Ogre::LogManager::getSingleton().logMessage("*** OgreOggSoundManager::createEFXEffect() - Unsupported effect!");
			return false;
		}

		ALuint effect;

		alGenEffects(1, &effect);
		if (alGetError() != AL_NO_ERROR)
		{
			Ogre::LogManager::getSingleton().logMessage("*** Cannot create EFX effect!");
			return false;
		}

		if (alIsEffect(effect))
		{
			alEffecti(effect, AL_EFFECT_TYPE, effectType);
			if (alGetError() != AL_NO_ERROR)
			{
				Ogre::LogManager::getSingleton().logMessage("*** Effect not supported!");
				return false;
			}
			else
			{
				// Apply some preset reverb properties
				if ( effectType==AL_EFFECT_EAXREVERB  && props )
				{
					EFXEAXREVERBPROPERTIES eaxProps;
					ConvertReverbParameters(props, &eaxProps);
					_setEAXReverbProperties(&eaxProps, effect);
				}

				// Add to list
				if ( !mEffectList ) mEffectList = new EffectList;
				(*mEffectList)[eName]=effect;
			}
		}
		return true;
	}

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::createEFXSlot()
	{
		if ( !hasEFXSupport() ) 
		{
			Ogre::LogManager::getSingleton().logMessage("*** OgreOggSoundManager::createEFXFilter() - No EFX support!");
			return false;
		}

		ALuint slot;

		alGenAuxiliaryEffectSlots(1, &slot);
		if (alGetError() != AL_NO_ERROR)
		{
			Ogre::LogManager::getSingleton().logMessage("*** Cannot create Auxiliary effect slot!");
			return false;
		}
		else
		{
			if ( !mEffectSlotList ) mEffectSlotList = new std::vector<ALuint>;
			mEffectSlotList->push_back(slot);
		}

		return true;
	}

	/*/////////////////////////////////////////////////////////////////*/
	int OgreOggSoundManager::getNumberOfSupportedEffectSlots()
	{
		if ( !hasEFXSupport() ) return 0;
		
		ALint auxSends=0;
		alcGetIntegerv(mDevice, ALC_MAX_AUXILIARY_SENDS, 1, &auxSends);

		return auxSends;
	}

	/*/////////////////////////////////////////////////////////////////*/
	int OgreOggSoundManager::getNumberOfCreatedEffectSlots()
	{
		if ( !mEffectSlotList ) return 0;

		return static_cast<int>(mEffectSlotList->size());
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
	bool OgreOggSoundManager::isEffectSupported(ALint effectID)
	{
		if ( mEFXSupportList.find(effectID)!=mEFXSupportList.end() )
			return mEFXSupportList[effectID];
		else
			Ogre::LogManager::getSingleton().logMessage("*** OgreOggSoundManager::isEffectSupported() - Invalid effectID!");

		return false;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::_determineAuxEffectSlots()
	{
		ALuint		uiEffectSlots[128] = { 0 };
		ALuint		uiEffects[1] = { 0 };
		ALuint		uiFilters[1] = { 0 };
		Ogre::String msg="";

		// To determine how many Auxiliary Effects Slots are available, create as many as possible (up to 128)
		// until the call fails.
		for (mNumEffectSlots = 0; mNumEffectSlots < 128; mNumEffectSlots++)
		{
			alGenAuxiliaryEffectSlots(1, &uiEffectSlots[mNumEffectSlots]);
			if (alGetError() != AL_NO_ERROR)
				break;
		}

		msg="*** --- "+Ogre::StringConverter::toString(mNumEffectSlots)+ " Auxiliary Effect Slot(s)"; 
		Ogre::LogManager::getSingleton().logMessage(msg);

		// Retrieve the number of Auxiliary Effect Slots Sends available on each Source
		alcGetIntegerv(mDevice, ALC_MAX_AUXILIARY_SENDS, 1, &mNumSendsPerSource);
		msg="*** --- "+Ogre::StringConverter::toString(mNumSendsPerSource)+" Auxiliary Send(s) per Source";
		Ogre::LogManager::getSingleton().logMessage(msg);

		Ogre::LogManager::getSingleton().logMessage("*** --- EFFECTS SUPPORTED:");
		alGenEffects(1, &uiEffects[0]);
		if (alGetError() == AL_NO_ERROR)
		{
			// Try setting Effect Type to known Effects
			alEffecti(uiEffects[0], AL_EFFECT_TYPE, AL_EFFECT_REVERB);
			if ( mEFXSupportList[AL_EFFECT_REVERB] = (alGetError() == AL_NO_ERROR) )
				Ogre::LogManager::getSingleton().logMessage("*** --- 'Reverb' Support: YES");
			else
				Ogre::LogManager::getSingleton().logMessage("*** --- 'Reverb' Support: NO");

			alEffecti(uiEffects[0], AL_EFFECT_TYPE, AL_EFFECT_EAXREVERB);
			if ( mEFXSupportList[AL_EFFECT_EAXREVERB] = (alGetError() == AL_NO_ERROR) )
				Ogre::LogManager::getSingleton().logMessage("*** --- 'EAX Reverb' Support: YES");
			else
				Ogre::LogManager::getSingleton().logMessage("*** --- 'EAX Reverb' Support: NO");

			alEffecti(uiEffects[0], AL_EFFECT_TYPE, AL_EFFECT_CHORUS);
			if ( mEFXSupportList[AL_EFFECT_CHORUS] = (alGetError() == AL_NO_ERROR) )
				Ogre::LogManager::getSingleton().logMessage("*** --- 'Chorus' Support: YES");
			else
				Ogre::LogManager::getSingleton().logMessage("*** --- 'Chorus' Support: NO");

			alEffecti(uiEffects[0], AL_EFFECT_TYPE, AL_EFFECT_DISTORTION);
			if ( mEFXSupportList[AL_EFFECT_DISTORTION] = (alGetError() == AL_NO_ERROR) )
				Ogre::LogManager::getSingleton().logMessage("*** --- 'Distortion' Support: YES");
			else
				Ogre::LogManager::getSingleton().logMessage("*** --- 'Distortion' Support: NO");

			alEffecti(uiEffects[0], AL_EFFECT_TYPE, AL_EFFECT_ECHO);
			if ( mEFXSupportList[AL_EFFECT_ECHO] = (alGetError() == AL_NO_ERROR) )
				Ogre::LogManager::getSingleton().logMessage("*** --- 'Echo' Support: YES");
			else
				Ogre::LogManager::getSingleton().logMessage("*** --- 'Echo' Support: NO");

			alEffecti(uiEffects[0], AL_EFFECT_TYPE, AL_EFFECT_FLANGER);
			if ( mEFXSupportList[AL_EFFECT_FLANGER] = (alGetError() == AL_NO_ERROR) )
				Ogre::LogManager::getSingleton().logMessage("*** --- 'Flanger' Support: YES");
			else
				Ogre::LogManager::getSingleton().logMessage("*** --- 'Flanger' Support: NO");

			alEffecti(uiEffects[0], AL_EFFECT_TYPE, AL_EFFECT_FREQUENCY_SHIFTER);
			if ( mEFXSupportList[AL_EFFECT_FREQUENCY_SHIFTER] = (alGetError() == AL_NO_ERROR) )
				Ogre::LogManager::getSingleton().logMessage("*** --- 'Frequency shifter' Support: YES");
			else
				Ogre::LogManager::getSingleton().logMessage("*** --- 'Frequency shifter' Support: NO");

			alEffecti(uiEffects[0], AL_EFFECT_TYPE, AL_EFFECT_VOCAL_MORPHER);
			if ( mEFXSupportList[AL_EFFECT_VOCAL_MORPHER] = (alGetError() == AL_NO_ERROR) )
				Ogre::LogManager::getSingleton().logMessage("*** --- 'Vocal Morpher' Support: YES");
			else
				Ogre::LogManager::getSingleton().logMessage("*** --- 'Vocal Morpher' Support: NO");

			alEffecti(uiEffects[0], AL_EFFECT_TYPE, AL_EFFECT_PITCH_SHIFTER);
			if ( mEFXSupportList[AL_EFFECT_PITCH_SHIFTER] = (alGetError() == AL_NO_ERROR) )
				Ogre::LogManager::getSingleton().logMessage("*** --- 'Pitch shifter' Support: YES");
			else
				Ogre::LogManager::getSingleton().logMessage("*** --- 'Pitch shifter' Support: NO");

			alEffecti(uiEffects[0], AL_EFFECT_TYPE, AL_EFFECT_RING_MODULATOR);
			if ( mEFXSupportList[AL_EFFECT_RING_MODULATOR] = (alGetError() == AL_NO_ERROR) )
				Ogre::LogManager::getSingleton().logMessage("*** --- 'Ring modulator' Support: YES");
			else
				Ogre::LogManager::getSingleton().logMessage("*** --- 'Ring modulator' Support: NO");

			alEffecti(uiEffects[0], AL_EFFECT_TYPE, AL_EFFECT_AUTOWAH);
			if ( mEFXSupportList[AL_EFFECT_AUTOWAH] = (alGetError() == AL_NO_ERROR) )
				Ogre::LogManager::getSingleton().logMessage("*** --- 'Autowah' Support: YES");
			else
				Ogre::LogManager::getSingleton().logMessage("*** --- 'Autowah' Support: NO");

			alEffecti(uiEffects[0], AL_EFFECT_TYPE, AL_EFFECT_COMPRESSOR);
			if ( mEFXSupportList[AL_EFFECT_COMPRESSOR] = (alGetError() == AL_NO_ERROR) )
				Ogre::LogManager::getSingleton().logMessage("*** --- 'Compressor' Support: YES");
			else
				Ogre::LogManager::getSingleton().logMessage("*** --- 'Compressor' Support: NO");

			alEffecti(uiEffects[0], AL_EFFECT_TYPE, AL_EFFECT_EQUALIZER);
			if ( mEFXSupportList[AL_EFFECT_EQUALIZER] = (alGetError() == AL_NO_ERROR) )
				Ogre::LogManager::getSingleton().logMessage("*** --- 'Equalizer' Support: YES");
			else
				Ogre::LogManager::getSingleton().logMessage("*** --- 'Equalizer' Support: NO");
		}

		
		// To determine which Filters are supported, generate a Filter Object, and try to set its type to
		// the various Filter enum values
		Ogre::LogManager::getSingleton().logMessage("*** --- FILTERS SUPPORTED: ");

		// Generate a Filter to use to determine what Filter Types are supported
		alGenFilters(1, &uiFilters[0]);
		if (alGetError() == AL_NO_ERROR)
		{
			// Try setting the Filter type to known Filters
			alFilteri(uiFilters[0], AL_FILTER_TYPE, AL_FILTER_LOWPASS);
			if ( mEFXSupportList[AL_FILTER_LOWPASS] = (alGetError() == AL_NO_ERROR) )
				Ogre::LogManager::getSingleton().logMessage("*** --- 'Low Pass' Support: YES");
			else
				Ogre::LogManager::getSingleton().logMessage("*** --- 'Low Pass' Support: NO");

			alFilteri(uiFilters[0], AL_FILTER_TYPE, AL_FILTER_HIGHPASS);
			if ( mEFXSupportList[AL_FILTER_HIGHPASS] = (alGetError() == AL_NO_ERROR) )
				Ogre::LogManager::getSingleton().logMessage("*** --- 'High Pass' Support: YES");
			else
				Ogre::LogManager::getSingleton().logMessage("*** --- 'High Pass' Support: NO");

			alFilteri(uiFilters[0], AL_FILTER_TYPE, AL_FILTER_BANDPASS);
			if ( mEFXSupportList[AL_FILTER_BANDPASS] = (alGetError() == AL_NO_ERROR) )
				Ogre::LogManager::getSingleton().logMessage("*** --- 'Band Pass' Support: YES");
			else
				Ogre::LogManager::getSingleton().logMessage("*** --- 'Band Pass' Support: NO");
		}

		// Delete Filter
		alDeleteFilters(1, &uiFilters[0]);

		// Delete Effect
		alDeleteEffects(1, &uiEffects[0]);

		// Delete Auxiliary Effect Slots
		alDeleteAuxiliaryEffectSlots(mNumEffectSlots, uiEffectSlots);
	}
	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::_checkEFXSupport()
	{
		if (alcIsExtensionPresent(mDevice, "ALC_EXT_EFX"))
		{
			// Get function pointers
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
				return true;
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
			_determineAuxEffectSlots();
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
	bool OgreOggSoundManager::_setEAXReverbProperties(EFXEAXREVERBPROPERTIES *pEFXEAXReverb, ALuint uiEffect)
	{
		if (pEFXEAXReverb)
		{
			// Clear AL Error code
			alGetError();
		
			// Determine type of 'Reverb' effect and apply correct settings
			ALint type;
			alGetEffecti(uiEffect, AL_EFFECT_TYPE, &type);
		
			// Apply selected presets to normal reverb 
			if ( type==AL_EFFECT_REVERB )
			{
				alEffectf(uiEffect, AL_REVERB_DENSITY, pEFXEAXReverb->flDensity);
				alEffectf(uiEffect, AL_REVERB_DIFFUSION, pEFXEAXReverb->flDiffusion);
				alEffectf(uiEffect, AL_REVERB_GAIN, pEFXEAXReverb->flGain);
				alEffectf(uiEffect, AL_REVERB_GAINHF, pEFXEAXReverb->flGainHF);
				alEffectf(uiEffect, AL_REVERB_DECAY_TIME, pEFXEAXReverb->flDecayTime);
				alEffectf(uiEffect, AL_REVERB_DECAY_HFRATIO, pEFXEAXReverb->flDecayHFRatio);
				alEffectf(uiEffect, AL_REVERB_REFLECTIONS_GAIN, pEFXEAXReverb->flReflectionsGain);
				alEffectf(uiEffect, AL_REVERB_REFLECTIONS_DELAY, pEFXEAXReverb->flReflectionsDelay);
				alEffectf(uiEffect, AL_REVERB_LATE_REVERB_GAIN, pEFXEAXReverb->flLateReverbGain);
				alEffectf(uiEffect, AL_REVERB_LATE_REVERB_DELAY, pEFXEAXReverb->flLateReverbDelay);
				alEffectf(uiEffect, AL_REVERB_AIR_ABSORPTION_GAINHF, pEFXEAXReverb->flAirAbsorptionGainHF);
				alEffectf(uiEffect, AL_REVERB_ROOM_ROLLOFF_FACTOR, pEFXEAXReverb->flRoomRolloffFactor);
				alEffecti(uiEffect, AL_REVERB_DECAY_HFLIMIT, pEFXEAXReverb->iDecayHFLimit);
			}
			// Apply full EAX reverb settings
			else
			{
				alEffectf(uiEffect, AL_EAXREVERB_DENSITY, pEFXEAXReverb->flDensity);
				alEffectf(uiEffect, AL_EAXREVERB_DIFFUSION, pEFXEAXReverb->flDiffusion);
				alEffectf(uiEffect, AL_EAXREVERB_GAIN, pEFXEAXReverb->flGain);
				alEffectf(uiEffect, AL_EAXREVERB_GAINHF, pEFXEAXReverb->flGainHF);
				alEffectf(uiEffect, AL_EAXREVERB_GAINLF, pEFXEAXReverb->flGainLF);
				alEffectf(uiEffect, AL_EAXREVERB_DECAY_TIME, pEFXEAXReverb->flDecayTime);
				alEffectf(uiEffect, AL_EAXREVERB_DECAY_HFRATIO, pEFXEAXReverb->flDecayHFRatio);
				alEffectf(uiEffect, AL_EAXREVERB_DECAY_LFRATIO, pEFXEAXReverb->flDecayLFRatio);
				alEffectf(uiEffect, AL_EAXREVERB_REFLECTIONS_GAIN, pEFXEAXReverb->flReflectionsGain);
				alEffectf(uiEffect, AL_EAXREVERB_REFLECTIONS_DELAY, pEFXEAXReverb->flReflectionsDelay);
				alEffectfv(uiEffect, AL_EAXREVERB_REFLECTIONS_PAN, pEFXEAXReverb->flReflectionsPan);
				alEffectf(uiEffect, AL_EAXREVERB_LATE_REVERB_GAIN, pEFXEAXReverb->flLateReverbGain);
				alEffectf(uiEffect, AL_EAXREVERB_LATE_REVERB_DELAY, pEFXEAXReverb->flLateReverbDelay);
				alEffectfv(uiEffect, AL_EAXREVERB_LATE_REVERB_PAN, pEFXEAXReverb->flLateReverbPan);
				alEffectf(uiEffect, AL_EAXREVERB_ECHO_TIME, pEFXEAXReverb->flEchoTime);
				alEffectf(uiEffect, AL_EAXREVERB_ECHO_DEPTH, pEFXEAXReverb->flEchoDepth);
				alEffectf(uiEffect, AL_EAXREVERB_MODULATION_TIME, pEFXEAXReverb->flModulationTime);
				alEffectf(uiEffect, AL_EAXREVERB_MODULATION_DEPTH, pEFXEAXReverb->flModulationDepth);
				alEffectf(uiEffect, AL_EAXREVERB_AIR_ABSORPTION_GAINHF, pEFXEAXReverb->flAirAbsorptionGainHF);
				alEffectf(uiEffect, AL_EAXREVERB_HFREFERENCE, pEFXEAXReverb->flHFReference);
				alEffectf(uiEffect, AL_EAXREVERB_LFREFERENCE, pEFXEAXReverb->flLFReference);
				alEffectf(uiEffect, AL_EAXREVERB_ROOM_ROLLOFF_FACTOR, pEFXEAXReverb->flRoomRolloffFactor);
				alEffecti(uiEffect, AL_EAXREVERB_DECAY_HFLIMIT, pEFXEAXReverb->iDecayHFLimit);
			}
			if (alGetError() == AL_NO_ERROR)
				return true;
		}

		return false;
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
	OgreOggISound* OgreOggSoundManager::createSound(const std::string& name, const std::string& file, bool stream, bool loop, bool preBuffer)
	{
#if OGGSOUND_THREADED
		boost::recursive_mutex::scoped_lock l(mMutex);
#endif

		Ogre::ResourceGroupManager* groupManager = Ogre::ResourceGroupManager::getSingletonPtr();
		Ogre::String group;
		Ogre::DataStreamPtr soundData;
		try
		{
			group = groupManager->findGroupContainingResource(file);
			soundData = groupManager->openResource(file, group);
		}
		catch (...)
		{
			// Cannot find sound file
			Ogre::LogManager::getSingleton().logMessage("***--- OgreOggSoundManager::createSound() - Unable to find sound file! have you specified a resource location?", Ogre::LML_CRITICAL);
			return (0);
		}

		if		(file.find(".ogg") != std::string::npos || file.find(".OGG") != std::string::npos)
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

			// Set loop flag
			mSoundMap[name]->loop(loop);

#if OGGSOUND_THREADED==0
			// Read audio file
			mSoundMap[name]->open(soundData);
			// If requested to preBuffer - grab free source and init
			if (preBuffer)
			{
				if ( !requestSoundSource(mSoundMap[name]) )
				{
					Ogre::String msg="*** OgreOggSoundManager::createSound() - Failed to preBuffer sound: "+name;
					Ogre::LogManager::getSingleton().logMessage(msg);
				}
			}
#else
			delayedFileOpen* fo = new delayedFileOpen;
			fo->mPrebuffer = preBuffer;
			fo->mFile = soundData;
			fo->mSound = mSoundMap[name];
			mQueuedSounds.push_back(fo);
#endif
			return mSoundMap[name];
		}
		else if	(file.find(".wav") != std::string::npos || file.find(".WAV") != std::string::npos)
		{
			// MUST be unique
			if ( mSoundMap.find(name)!=mSoundMap.end() ) 
			{
				Ogre::String msg="*** OgreOggSoundManager::createSound() - Sound with name: "+name+" already exists!";
				Ogre::LogManager::getSingleton().logMessage(msg);
				return 0;
			}

			if(stream)
				mSoundMap[name] = new OgreOggStreamWavSound(name);
			else 
				mSoundMap[name] = new OgreOggStaticWavSound(name);

			// Set loop flag
			mSoundMap[name]->loop(loop);

#if OGGSOUND_THREADED==0
			// Read audio file
			mSoundMap[name]->open(soundData);
			// If requested to preBuffer - grab free source and init
			if (preBuffer)
			{
				if ( !requestSoundSource(mSoundMap[name]) )
				{
					Ogre::String msg="*** OgreOggSoundManager::createSound() - Failed to preBuffer sound: "+name;
					Ogre::LogManager::getSingleton().logMessage(msg);
				}
			}
#else
			delayedFileOpen* fo = new delayedFileOpen;
			fo->mPrebuffer = preBuffer;
			fo->mFile = soundData;
			fo->mSound = mSoundMap[name];
			mQueuedSounds.push_back(fo);
#endif
			return mSoundMap[name];
		}
		else
		{
			Ogre::String msg="*** OgreOggSoundManager::createSound() - Sound does not have .ogg extension: "+name;
			Ogre::LogManager::getSingleton().logMessage(msg);
			return NULL;
		}
	}

	/*/////////////////////////////////////////////////////////////////*/
	OgreOggISound* OgreOggSoundManager::createSound(SceneManager& scnMgr, const std::string& name, const std::string& file, bool stream, bool loop, bool preBuffer)
	{
#if OGGSOUND_THREADED
		boost::recursive_mutex::scoped_lock l(mMutex);
#endif

		Ogre::NameValuePairList params;
		params["fileName"]	= file;
		params["stream"]	= stream	? "true" : "false";
		params["loop"]		= loop		? "true" : "false";
		params["preBuffer"]	= preBuffer ? "true" : "false";

		OgreOggISound* sound = 0;

		// Catch exception when plugin hasn't been registered
		try
		{
			sound = static_cast<OgreOggISound*>(scnMgr.createMovableObject( name, OgreOggSoundFactory::FACTORY_TYPE_NAME, &params ));		
			sound->mScnMan = &scnMgr;
		}
		catch (...)
		{
			LogManager::getSingleton().logMessage("***--- createSound() - OgreOggSound plugin not loaded.");
		}
		// create Movable Sound
		return sound; 
	}

	/*/////////////////////////////////////////////////////////////////*/
	OgreOggISound* OgreOggSoundManager::getSound(const std::string& name)
	{

#if OGGSOUND_THREADED
		boost::recursive_mutex::scoped_lock l(mMutex);
#endif

		SoundMap::iterator i = mSoundMap.find(name);
		if(i == mSoundMap.end()) return 0;
		return i->second;
	}
	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::setEFXSoundProperties(const std::string& sName, Ogre::Real airAbsorption, Ogre::Real roomRolloff, Ogre::Real coneOuterHF)
	{
#if OGGSOUND_THREADED
		boost::recursive_mutex::scoped_lock l(mMutex);
#endif

		OgreOggISound* sound = getSound(sName);

		if ( sound )
		{
			ALuint src = sound->getSource();

			if ( src!=AL_NONE )
			{
				alGetError();

				alSourcef(src, AL_AIR_ABSORPTION_FACTOR, airAbsorption);
				alSourcef(src, AL_ROOM_ROLLOFF_FACTOR, roomRolloff);
				alSourcef(src, AL_CONE_OUTER_GAINHF, coneOuterHF);

				if ( alGetError()==AL_NO_ERROR )
				{
					return true;
				}
				else
				{
					Ogre::LogManager::getSingleton().logMessage("*** OgreOggSoundManager::setEFXSoundProperties() - Unable to set EFX sound properties!");
					return false;
				}
			}
			else
			{
				Ogre::LogManager::getSingleton().logMessage("*** OgreOggSoundManager::setEFXSoundProperties() - No source attached to sound!");
				return false;
			}
		}

		Ogre::LogManager::getSingleton().logMessage("*** OgreOggSoundManager::setEFXSoundProperties() - Sound does not exist!");
		return false;
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
	void OgreOggSoundManager::playSound(const String& sName)
	{
	#if OGGSOUND_THREADED
		boost::recursive_mutex::scoped_lock l(mMutex);
	#endif

		OgreOggISound* sound = 0;
		
		if ( sound = getSound(sName) )
			sound->play();
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::stopSound(const String& sName)
	{
	#if OGGSOUND_THREADED
		boost::recursive_mutex::scoped_lock l(mMutex);
	#endif

		if (mActiveSounds.empty()) return;

		OgreOggISound* sound = 0;
		
		if ( sound = getSound(sName) )
			sound->stop();
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::pauseSound(const String& sName)
	{
	#if OGGSOUND_THREADED
		boost::recursive_mutex::scoped_lock l(mMutex);
	#endif

		if (mActiveSounds.empty()) return;

		OgreOggISound* sound = 0;
		
		if ( sound = getSound(sName) )
			sound->pause();
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
			{
				// Pause sound
				(*iter)->pause();

				// Add to list to allow resuming
				mPausedSounds.push_back((*iter));
			}
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
	void OgreOggSoundManager::_destroy(OgreOggISound* sound)
	{
		if (!sound) return;

	#if OGGSOUND_THREADED
		boost::recursive_mutex::scoped_lock l(mMutex);
	#endif

		// Find sound in map
		SoundMap::iterator i = mSoundMap.find(sound->getName());
		if (i != mSoundMap.end())
		{
			// Remove from reactivate list
			if ( !mSoundsToReactivate.empty() )
			{
				for ( ActiveList::iterator iter=mSoundsToReactivate.begin(); iter!=mSoundsToReactivate.end(); ++iter )
					if ( sound==(*iter) )
					{
						mSoundsToReactivate.erase(iter);
						break;
					}
			}

			// Delete sound
			ALuint src = sound->getSource();
			if ( src!=AL_NONE ) releaseSoundSource(sound);

			// Delete sounds memory
			delete sound;

			// Remove from sound list
			mSoundMap.erase(i);
		}
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::destroySound(const Ogre::String& sName)
	{
		if ( sName.empty() ) return;

		// Find sound in map
		SoundMap::iterator i = mSoundMap.find(sName);
		if (i != mSoundMap.end())
		{
			// If created via plugin call destroyMovableObject() which will call _destroy()
			if (i->second->mScnMan)
				i->second->mScnMan->destroyMovableObject(i->second->getName(), OgreOggSoundFactory::FACTORY_TYPE_NAME);
			// else call _destroy() directly
			else
				_destroy(i->second);
		}
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::processQueuedSounds()
	{
	#if OGGSOUND_THREADED
		boost::recursive_mutex::scoped_lock l(mMutex);
	#endif
		if (mQueuedSounds.empty()) return;

		FileOpenList::iterator i = mQueuedSounds.begin();
		while( i != mQueuedSounds.end())
		{
			// Open file for reading
			(*i)->mSound->open((*i)->mFile);

			// Prebuffer if requested
			if ((*i)->mPrebuffer ) requestSoundSource((*i)->mSound);

			// Remove from queue
			delete (*i);
			i=mQueuedSounds.erase(i);
		}	
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::updateBuffers()
	{
	#if OGGSOUND_THREADED
		boost::recursive_mutex::scoped_lock l(mMutex);
	#endif
		ActiveList::iterator i = mActiveSounds.begin();
		while( i != mActiveSounds.end())
		{
			(*i)->_updateAudioBuffers();
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
			if ( hasEFXSupport() )
			{
				// Remove filters/effects
				alSourcei(static_cast<ALuint>((*it)), AL_DIRECT_FILTER, AL_FILTER_NULL);
				alSource3i(static_cast<ALuint>((*it)), AL_AUXILIARY_SEND_FILTER, AL_EFFECTSLOT_NULL, 0, AL_FILTER_NULL);
 			}
			alDeleteSources(1,&(*it));
			++it;
		}

		mSourcePool.clear();

		// clear EFX effect lists
		if ( mFilterList )
		{
			for ( EffectList::iterator iter=mFilterList->begin(); iter!=mFilterList->end(); ++iter )
			    alDeleteEffects( 1, &iter->second);
			delete mFilterList;
			mFilterList=0;
		}

		if ( mEffectList )
		{
			for ( EffectList::iterator iter=mEffectList->begin(); iter!=mEffectList->end(); ++iter )
			    alDeleteEffects( 1, &iter->second);
			delete mEffectList;
			mEffectList=0;
		}

		if ( mEffectSlotList )
		{
			for ( std::vector<ALuint>::iterator iter=mEffectSlotList->begin(); iter!=mEffectSlotList->end(); ++iter )
			    alDeleteEffects( 1, &(*iter));
			delete mEffectSlotList;
			mEffectSlotList=0;
		}
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
	struct OgreOggSoundManager::_sortNearToFar
	{
		bool operator()(OgreOggISound* sound1, OgreOggISound* sound2)
		{
			Real	d1=0.f,
					d2=0.f;
			Vector3	lPos=OgreOggSoundManager::getSingleton().getListener()->getPosition();

			if ( sound1->isRelativeToListener() )
				d1 = sound1->getPosition().length();
			else
				d1 = sound1->getPosition().distance(lPos);

			if ( sound2->isRelativeToListener() )
				d2 = sound2->getPosition().length();
			else
				d2 = sound2->getPosition().distance(lPos);

			// Check sort order
			if ( d1<d2 )	return true;
			if ( d1>d2 )	return false;

			// Equal - don't sort 
			return false;
		}
	};
	/*/////////////////////////////////////////////////////////////////*/
	struct OgreOggSoundManager::_sortFarToNear
	{
		bool operator()(OgreOggISound* sound1, OgreOggISound* sound2)
		{
			Real	d1=0.f,
					d2=0.f;
			Vector3	lPos=OgreOggSoundManager::getSingleton().getListener()->getPosition();

			if ( sound1->isRelativeToListener() )
				d1 = sound1->getPosition().length();
			else
				d1 = sound1->getPosition().distance(lPos);

			if ( sound2->isRelativeToListener() )
				d2 = sound2->getPosition().length();
			else
				d2 = sound2->getPosition().distance(lPos);

			// Check sort order
			if ( d1>d2 )	return true;
			if ( d1<d2 )	return false;

			// Equal - don't sort 
			return false;
		}
	};
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::_reactivateQueuedSounds()
	{
		// Any sounds to re-activate?
		if (mSoundsToReactivate.empty()) return;

	#if OGGSOUND_THREADED
		boost::recursive_mutex::scoped_lock l(mMutex);
	#endif

		// Sort list by distance
		std::sort(mSoundsToReactivate.begin(), mSoundsToReactivate.end(), _sortNearToFar());

		// Get sound object from front of list
		OgreOggISound* snd = mSoundsToReactivate.front(); 

		// Try to request a source for sound
		if (requestSoundSource(snd))
		{
			// Request new source for reactivated sound
			snd->play();
		}
	}
	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::requestSoundSource(OgreOggISound* sound)
	{
		// Does sound need a source?
		if (!sound) return false;
		
	#if OGGSOUND_THREADED
		boost::recursive_mutex::scoped_lock l(mMutex);
	#endif

		if (sound->getSource()!=AL_NONE) return true;

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
			// Remove from reactivate list if reactivating..
			if ( !mSoundsToReactivate.empty() )
			{
				for ( ActiveList::iterator rIter=mSoundsToReactivate.begin(); rIter!=mSoundsToReactivate.end(); ++rIter )
					if ( (*rIter)==sound ) 
						mSoundsToReactivate.erase(rIter);
			}
			return true;
		}
		// All sources in use 
		// Re-use an active source
		// Use either a non-playing source or a lower priority source
		else
		{
			// Get iterator for list
			ActiveList::iterator iter = mActiveSounds.begin();

			// Search for a stopped sound
			while ( iter!=mActiveSounds.end() )
			{
				// Find a stopped sound - reuse its source
				if ( (*iter)->isStopped() )
				{
					ALuint src = (*iter)->getSource();
					ALuint nullSrc = AL_NONE;
					// Pause sounds
					(*iter)->pause();
					// Remove source
					(*iter)->setSource(nullSrc);
					// Attach source to new sound
					sound->setSource(src);
					// Add to reactivate list
					mSoundsToReactivate.push_back((*iter));
					// Remove relinquished sound from active list
					mActiveSounds.erase(iter);
					// Add new sound to active list
					mActiveSounds.push_back(sound);
					// Return success
					return true;
				}
				else
					++iter;
			}

			// Check priority...
			Ogre::uint8 priority = sound->getPriority();
			iter = mActiveSounds.begin();

			// Search for a stopped sound
			while ( iter!=mActiveSounds.end() )
			{
				// Find a stopped sound - reuse its source
				if ( (*iter)->getPriority()<sound->getPriority() )
				{
					ALuint src = (*iter)->getSource();
					ALuint nullSrc = AL_NONE;
					// Pause sounds
					(*iter)->pause();
					// Remove source
					(*iter)->setSource(nullSrc);
					// Attach source to new sound
					sound->setSource(src);
					// Add to reactivate list
					mSoundsToReactivate.push_back((*iter));
					// Remove relinquished sound from active list
					mActiveSounds.erase(iter);
					// Add new sound to active list
					mActiveSounds.push_back(sound);
					// Return success
					return true;
				}
				else
					++iter;
			}
			
			// Sort by distance
			Real	d1 = 0.f,
					d2 = 0.f;

			// Sort list by distance
			std::sort(mActiveSounds.begin(), mActiveSounds.end(), _sortFarToNear());

			// Lists should be sorted:	Active-->furthest to Nearest
			//							Reactivate-->Nearest to furthest
			OgreOggISound* snd1 = mActiveSounds.front();

			if ( snd1->isRelativeToListener() )
				d1 = snd1->getPosition().length();
			else
				d1 = snd1->getPosition().distance(mListener->getPosition());

			if ( sound->isRelativeToListener() )
				d1 = sound->getPosition().length();
			else
				d1 = sound->getPosition().distance(mListener->getPosition());

			// Needs swapping?
			if ( d1>d2 )
			{
				ALuint src = snd1->getSource();
				ALuint nullSrc = AL_NONE;
				// Pause sounds
				snd1->pause();
				// Remove source
				snd1->setSource(nullSrc);
				// Attach source to new sound
				sound->setSource(src);
				// Add to reactivate list
				mSoundsToReactivate.push_back(snd1);
				// Remove relinquished sound from active list
				mActiveSounds.erase(mActiveSounds.begin());
				// Add new sound to active list
				mActiveSounds.push_back(sound);
				// Return success
				return true;
			}
		}
		// Uh oh - won't be played
		return false;
	}

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggSoundManager::releaseSoundSource(OgreOggISound* sound)
	{
		if (!sound) return false;

	#if OGGSOUND_THREADED
		boost::recursive_mutex::scoped_lock l(mMutex);
	#endif
		
		if (sound->getSource()==AL_NONE) return true;
			
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
	void OgreOggSoundManager::setEFXDistanceUnits(Ogre::Real units)
	{
		if ( !hasEFXSupport() || units<=0 ) return;

		alListenerf(AL_METERS_PER_UNIT, units);
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggSoundManager::update(Ogre::Real fTime)
	{		
		static Real rTime=0.f;

		// Update ALL active sounds
		ActiveList::iterator i = mActiveSounds.begin();
		while( i != mActiveSounds.end())
		{
			(*i)->update(fTime);
	#if ( OGGSOUND_THREADED==0 )
			(*i)->_updateAudioBuffers();
	#endif
			i++;
		}

		// Limit re-activation 
		if ( (rTime+=fTime) > 0.1 )
		{
			// try to reactivate any 
			_reactivateQueuedSounds();

			// Reset timer
			rTime=0.f;
		}

		// Update listener
		mListener->update();
	}
}