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

#include "OgreOggStaticSound.h"
#include <string>
#include <iostream>
#include "OgreOggSound.h"

using namespace std;

namespace OgreOggSound
{

	/*/////////////////////////////////////////////////////////////////*/
	OgreOggStaticSound::OgreOggStaticSound(const Ogre::String& name) : OgreOggISound(name)
	,mVorbisInfo(0)
	,mVorbisComment(0)
	,mPreviousOffset(0)
	,mAudioName("")
	,mBuffer(0)
	{
		mStream=false;
	}
	/*/////////////////////////////////////////////////////////////////*/
	OgreOggStaticSound::~OgreOggStaticSound()
	{
		_release();
		mVorbisInfo=0;
		mVorbisComment=0;
		mBufferData.clear();
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStaticSound::open(Ogre::DataStreamPtr& fileStream)
	{
		int result;

		// Store stream pointer
		mAudioStream = fileStream;

		// Store file name
		mAudioName = mAudioStream->getName();

		if((result = ov_open_callbacks(&mAudioStream, &mOggStream, NULL, 0, mOggCallbacks)) < 0)
		{
			throw string("Could not open Ogg stream. ");
			return;
		}


		// Seekable file?
		if(ov_seekable(&mOggStream)==0)
		{
			// Disable seeking
			mSeekable = false;
		}

		mVorbisInfo = ov_info(&mOggStream, -1);
		mVorbisComment = ov_comment(&mOggStream, -1);

		// Get playtime in secs
		mPlayTime = ov_time_total(&mOggStream, -1);

		// Check format support
		if (!_queryBufferInfo())
			throw std::string("Format NOT supported!");

		alGenBuffers(1, &mBuffer);

		char* data;
		int sizeRead = 0;
		int bitStream;

		data = OGRE_ALLOC_T(char, mBufferSize, Ogre::MEMCATEGORY_GENERAL);
		do
		{
			sizeRead = ov_read(&mOggStream, data, static_cast<int>(mBufferSize), 0, 2, 1, &bitStream);
			mBufferData.insert(mBufferData.end(), data, data + sizeRead);
		}
		while(sizeRead > 0);
		OGRE_FREE(data, Ogre::MEMCATEGORY_GENERAL);

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
		// Upload to XRAM buffers if available
		if ( OgreOggSoundManager::getSingleton().hasXRamSupport() )
			OgreOggSoundManager::getSingleton().setXRamBuffer(1, &mBuffer);
#endif

		alGetError();
		alBufferData(mBuffer, mFormat, &mBufferData[0], static_cast<ALsizei>(mBufferData.size()), mVorbisInfo->rate);
		if ( alGetError()!=AL_NO_ERROR )
		{
			Ogre::LogManager::getSingleton().logMessage("*** --- OgreOggStaticSound::open() - Unable to load audio data into buffer!!", Ogre::LML_CRITICAL);
			throw std::string("Unable to load buffers with data!");
		}

		// Register shared buffer
		OgreOggSoundManager::getSingleton().registerSharedBuffer(mAudioName, mBuffer);

		// Ready for playback
		mFileOpened = true;
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStaticSound::open(const Ogre::String& fName, ALuint& buffer)
	{
		// Set buffer
		mBuffer = buffer;

		// Filename
		mAudioName = fName;

		// Ready for playback
		mFileOpened = true;
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStaticSound::_release()
	{
		ALuint src=AL_NONE;
		setSource(src);
		OgreOggSoundManager::getSingleton().releaseSharedBuffer(mAudioName, mBuffer);
		if ( !mAudioStream.isNull() ) ov_clear(&mOggStream);
		mPlayPosChanged = false;
		mPlayPos = 0.f;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStaticSound::_prebuffer()
	{
		if (mSource==AL_NONE) return;

		// Queue buffer
		alSourcei(mSource, AL_BUFFER, mBuffer);
	}

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggStaticSound::_queryBufferInfo()
	{
		if (!mVorbisInfo)
		{
			Ogre::LogManager::getSingleton().logMessage("*** --- No vorbis info!");
			return false;
		}

		switch(mVorbisInfo->channels)
		{
		case 1:
			{
				mFormat = AL_FORMAT_MONO16;
				// Set BufferSize to 250ms (Frequency * 2 (16bit) divided by 4 (quarter of a second))
				mBufferSize = mVorbisInfo->rate >> 1;
				// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
				mBufferSize -= (mBufferSize % 2);
			}
			break;
		case 2:
			{
				mFormat = AL_FORMAT_STEREO16;
				// Set BufferSize to 250ms (Frequency * 4 (16bit stereo) divided by 4 (quarter of a second))
				mBufferSize = mVorbisInfo->rate;
				// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
				mBufferSize -= (mBufferSize % 4);
			}
			break;
		case 4:
			{
				mFormat = alGetEnumValue("AL_FORMAT_QUAD16");
				if (!mFormat) return false;
				// Set BufferSize to 250ms (Frequency * 8 (16bit 4-channel) divided by 4 (quarter of a second))
				mBufferSize = mVorbisInfo->rate * 2;
				// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
				mBufferSize -= (mBufferSize % 8);
			}
			break;
		case 6:
			{
				mFormat = alGetEnumValue("AL_FORMAT_51CHN16");
				if (!mFormat) return false;
				// Set BufferSize to 250ms (Frequency * 12 (16bit 6-channel) divided by 4 (quarter of a second))
				mBufferSize = mVorbisInfo->rate * 3;
				// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
				mBufferSize -= (mBufferSize % 12);
			}
			break;
		case 7:
			{
				mFormat = alGetEnumValue("AL_FORMAT_61CHN16");
				if (!mFormat) return false;
				// Set BufferSize to 250ms (Frequency * 16 (16bit 7-channel) divided by 4 (quarter of a second))
				mBufferSize = mVorbisInfo->rate * 4;
				// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
				mBufferSize -= (mBufferSize % 16);
			}
			break;
		case 8:
			{
				mFormat = alGetEnumValue("AL_FORMAT_71CHN16");
				if (!mFormat) return false;
				// Set BufferSize to 250ms (Frequency * 20 (16bit 8-channel) divided by 4 (quarter of a second))
				mBufferSize = mVorbisInfo->rate * 5;
				// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
				mBufferSize -= (mBufferSize % 20);
			}
			break;
		default:
			// Couldn't determine buffer format so log the error and default to mono
			Ogre::LogManager::getSingleton().logMessage("!!WARNING!! Could not determine buffer format!  Defaulting to MONO");

			mFormat = AL_FORMAT_MONO16;
			// Set BufferSize to 250ms (Frequency * 2 (16bit) divided by 4 (quarter of a second))
			mBufferSize = mVorbisInfo->rate >> 1;
			// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
			mBufferSize -= (mBufferSize % 2);
			break;
		}
		return true;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStaticSound::setSource(ALuint& src)
	{
		if (src!=AL_NONE)
		{
			// Attach new source
			mSource=src;

			// Load audio data onto source
			_prebuffer();

			// Init source properties
			_initSource();
		}
		else
		{
			// Need to stop sound BEFORE unqueuing
			alSourceStop(mSource);

			// Unqueue buffer
			alSourcei(mSource, AL_BUFFER, 0);

			// Attach new source
			mSource=src;
		}
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStaticSound::pause()
	{
#if OGGSOUND_THREADED

		// If threaded it may be possible that a sound is trying to be played
		// before its actually been opened by the thread, if so mark it so
		// that it can be automatically played when ready.
		if (!mFileOpened)
		{
			if ( !mPauseDelayed )
			{
				mPauseDelayed = true;
				// Register this sound with the manager so it can be polled to play when ready
				OgreOggSoundManager::getSingletonPtr()->queueDelayedSound(this, DA_PAUSE);
			}
			return;
		}
#endif

		if ( mSource==AL_NONE ) return;

		alSourcePause(mSource);

		mPauseDelayed=false;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStaticSound::play()
	{
		if(isPlaying())
			return;

#if OGGSOUND_THREADED

		// If threaded it may be possible that a sound is trying to be played
		// before its actually been opened by the thread, if so mark it so
		// that it can be automatically played when ready.
		if (!mFileOpened)
		{
			if ( !mPlayDelayed )
			{
				mPlayDelayed = true;
				// Register this sound with the manager so it can be polled to play when ready
				OgreOggSoundManager::getSingletonPtr()->queueDelayedSound(this, DA_PLAY);
			}
			return;
		}
#endif

		if (mSource == AL_NONE)
			if ( !OgreOggSoundManager::getSingleton().requestSoundSource(this) )
				return;

		// Pick up playback position change..
		if ( mPlayPosChanged )
			setPlayPosition(mPlayPos);

		alSourcePlay(mSource);
		mPlay = true;
		mPlayDelayed=false;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStaticSound::stop()
	{
#if OGGSOUND_THREADED

		// If threaded it may be possible that a sound is trying to be played
		// before its actually been opened by the thread, if so mark it so
		// that it can be automatically played when ready.
		if (!mFileOpened)
		{
			if ( !mPlayDelayed )
			{
				mPlayDelayed = true;
				// Register this sound with the manager so it can be polled to play when ready
				OgreOggSoundManager::getSingletonPtr()->queueDelayedSound(this, DA_PLAY);
			}
			return;
		}
#endif

		if ( mSource==AL_NONE ) return;

		alSourceStop(mSource);
		alSourceRewind(mSource);
		mPlay=false;
		mPreviousOffset=0;
		mStopDelayed=false;

		// Give up source immediately if specfied
		if (mGiveUpSource) 
			OgreOggSoundManager::getSingleton().releaseSoundSource(this);
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStaticSound::loop(bool loop)
	{
		OgreOggISound::loop(loop);

		if(mSource != AL_NONE)
		{
			alSourcei(mSource,AL_LOOPING, loop);
		}
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStaticSound::_updateAudioBuffers()
	{
		// Automatically play if ready.
		if (mPlayDelayed)
			play();

		if(mSource == AL_NONE || !mPlay)
			return;

		ALenum state;
		alGetSourcei(mSource, AL_SOURCE_STATE, &state);

		if (state == AL_STOPPED)
		{
			stop();
			// Finished callback
			if ( mFinishedCB && mFinCBEnabled )
				mFinishedCB->execute(static_cast<OgreOggISound*>(this));
		}
		else
		{
			ALint bytes=0;

			// Use byte offset to work out current position
			alGetSourcei(mSource, AL_BYTE_OFFSET, &bytes);

			// Has the audio looped?
			if ( mPreviousOffset>bytes )
			{
				if ( mLoopCB && mLoopCBEnabled )
					mLoopCB->execute(static_cast<OgreOggISound*>(this));
			}

			// Store current offset position
			mPreviousOffset=bytes;
		}
	}
}
