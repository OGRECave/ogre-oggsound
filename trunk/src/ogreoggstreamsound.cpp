/**
* @file OgreOggStreamSound.cpp
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
*/

#include "OgreOggStreamSound.h"
#include <string>
#include <iostream>
#include "OgreOggSoundManager.h"

using namespace std;

namespace OgreOggSound
{
	/*/////////////////////////////////////////////////////////////////*/
	OgreOggStreamSound::OgreOggStreamSound(const Ogre::String& name) : OgreOggISound(name)
	,mVorbisInfo(0)
	,mVorbisComment(0)
	,mStreamEOF(false)
	{
		mStream=true;
		for ( int i=0; i<NUM_BUFFERS; i++ ) mBuffers[i]=AL_NONE; 
	}
	/*/////////////////////////////////////////////////////////////////*/
	OgreOggStreamSound::~OgreOggStreamSound()
	{
		_release();
		mVorbisInfo=0;
		mVorbisComment=0;
		for ( int i=0; i<NUM_BUFFERS; i++ ) mBuffers[i]=0;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamSound::_openImpl(Ogre::DataStreamPtr& fileStream)
	{
		int result;

		// Store stream pointer
		mAudioStream = fileStream;

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

		// Get total playtime in seconds
		mPlayTime = ov_time_total(&mOggStream, -1);

		// Generate audio buffers
		alGenBuffers(NUM_BUFFERS, mBuffers);

			// Check format support
		if (!_queryBufferInfo())
			throw std::string("Format NOT supported!");

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
		// Upload to XRAM buffers if available
		if ( OgreOggSoundManager::getSingleton().hasXRamSupport() )
			OgreOggSoundManager::getSingleton().setXRamBuffer(NUM_BUFFERS, mBuffers);
#endif
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamSound::_release()
	{
		ALuint src=AL_NONE;
		setSource(src);
		for (int i=0; i<NUM_BUFFERS; i++)
		{
			if (mBuffers[i]!=AL_NONE)
				alDeleteBuffers(1, &mBuffers[i]);
		}
		if ( !mAudioStream.isNull() ) ov_clear(&mOggStream);
		mPlayPosChanged = false;
		mPlayPos = 0.f;
	}
	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggStreamSound::_queryBufferInfo()
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
	void OgreOggStreamSound::_prebuffer()
	{
		if (mSource==AL_NONE) return;

		int i=0;
		while ( i<NUM_BUFFERS )
		{
			if ( _stream(mBuffers[i]) )
				alSourceQueueBuffers(mSource, 1, &mBuffers[i++]);
			else
				break;
		}
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamSound::setSource(ALuint& src)
	{
		if (src!=AL_NONE)
		{
			// Set source
			mSource=src;

			// Fill data buffers
			_prebuffer();

			// Init source
			_initSource();
		}
		else
		{
			// Unqueue buffers
			_dequeue();

			// Set source
			mSource=src;
		}
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamSound::_updateAudioBuffers()
	{
		if (mSource == AL_NONE || !mPlay) return;

		ALenum state;
		alGetSourcei(mSource, AL_SOURCE_STATE, &state);

		if (state == AL_PAUSED) return;

		// Ran out of buffer data?
		if (state == AL_STOPPED)
		{
			if(mStreamEOF)
			{
				stop();
				// Finished callback
				if ( mFinishedCB && mFinCBEnabled )
					mFinishedCB->execute(static_cast<OgreOggISound*>(this));
				return;
			}
			else
			{
				// Clear audio data already played...
				_dequeue();

				// Fill with next chunk of audio...
				_prebuffer();

				// Play...
				alSourcePlay(mSource);
			}
		}

		int processed;

		alGetSourcei(mSource, AL_BUFFERS_PROCESSED, &processed);

		while(processed--)
		{
			ALuint buffer;

			alSourceUnqueueBuffers(mSource, 1, &buffer);
			if ( _stream(buffer) ) alSourceQueueBuffers(mSource, 1, &buffer);
		}

		// handle play position change 
		if ( mPlayPosChanged ) 
		{
			_updatePlayPosition();
		}
	}
	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggStreamSound::_stream(ALuint buffer)
	{
		std::vector<char> audioData;
		char* data;
		int  bytes = 0;
		int  section = 0;
		int  result = 0;

		// Create buffer
		data = OGRE_ALLOC_T(char, mBufferSize, Ogre::MEMCATEGORY_GENERAL);
		
		// Read only what was asked for
		while(static_cast<int>(audioData.size()) < mBufferSize)
		{
			// Read up to a buffer's worth of data
			bytes = ov_read(&mOggStream, data, static_cast<int>(mBufferSize), 0, 2, 1, &section);
			// EOF check
			if (bytes == 0)
			{
				// If set to loop wrap to start of stream
				if ( mLoop && mSeekable )
				{
					if ( ov_time_seek(&mOggStream, 0)!= 0 )
					{
						Ogre::LogManager::getSingleton().logMessage("***--- OgreOggStream::_stream() - ERROR looping stream, ogg file NOT seekable!");
						break;
					}
					/**	This is the closest we can get to a loop trigger.
						If, whilst filling the buffers, we need to wrap the stream
						pointer, trigger the loop callback if defined.
						NOTE:- The accuracy of this method will be affected by a number of
						parameters, namely the buffer size, whether the sound has previously
						given up its source (therefore it will be re-filling all buffers, which,
						if the sound was close to eof will likely get triggered), and the quality
						of the sound, lower quality will hold a longer section of audio per buffer.
						In ALL cases this trigger will happen BEFORE the audio audibly loops!!
					*/
					if ( mLoopCB && mLoopCBEnabled )
						mLoopCB->execute(static_cast<OgreOggISound*>(this));
				}
				else
				{
					mStreamEOF=true;
					// Don't loop - finish.
					break;
				}
			}
			// Append to end of buffer
			audioData.insert(audioData.end(), data, data + bytes);
			// Keep track of read data
			result+=bytes;
		}

		// EOF
		if(result == 0)
		{
			OGRE_FREE(data, Ogre::MEMCATEGORY_GENERAL);
			return false;
		}

		alGetError();
		// Copy buffer data
		alBufferData(buffer, mFormat, &audioData[0], static_cast<ALsizei>(audioData.size()), mVorbisInfo->rate);

		// Cleanup
		OGRE_FREE(data, Ogre::MEMCATEGORY_GENERAL);

		return true;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamSound::_dequeue()
	{
		if(mSource == AL_NONE)
			return;

		/** Check current state
		@remarks
			Fix for bug where prebuffering a streamed sound caused a buffer problem
			resulting in only 1st buffer repeatedly looping. This is because alSourceStop() 
			doesn't function correctly if the sources state hasn't previously been set!!???
		*/
		ALenum state;
		alGetSourcei(mSource, AL_SOURCE_STATE, &state);

		// Force mSource to change state so the call to alSourceStop() will mark buffers correctly.
		if (state == AL_INITIAL)
			alSourcePlay(mSource);

		// Stop source to allow unqueuing
		alSourceStop(mSource);

		int queued=0;

		// Get number of buffers queued on source
		alGetError();
		alGetSourcei(mSource, AL_BUFFERS_PROCESSED, &queued);

		while (queued--)
		{
			ALuint buffer;
			// Remove number of buffers from source
			alSourceUnqueueBuffers(mSource, 1, &buffer);

			// Any problems?
			if ( alGetError()!=AL_NO_ERROR ) Ogre::LogManager::getSingleton().logMessage("*** Unable to unqueue buffers");
		}
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamSound::_updatePlayPosition()
	{
		if ( mSource==AL_NONE || !mSeekable ) 
			return;

		bool paused = isPlaying();
		bool playing = isPaused();

		// Seek...
		pause();
		ov_time_seek(&mOggStream, mPlayPos);

		// Unqueue all buffers
		_dequeue();

		// Fill buffers
		_prebuffer();

		// Reset state..
		if		( playing ) play();
		else if ( paused ) pause();

		// Set flag
		mPlayPosChanged = false;
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamSound::setPlayPosition(Ogre::Real seconds)
	{
		if ( !mSeekable || seconds<0.f ) 
			return;

		// Wrap time
		if ( seconds>mPlayTime )
		{
			do { seconds-=mPlayTime; } while ( seconds>mPlayTime );
		}

		// Set position
		mPlayPos = seconds;
	
		// Set flag
		mPlayPosChanged = true;
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamSound::_pauseImpl()
	{
		if(mSource == AL_NONE) return;

		alSourcePause(mSource);
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamSound::_playImpl()
	{
		if (isPlaying())
			return;

		// Grab a source if not already attached
		if (mSource == AL_NONE)
			if ( !OgreOggSoundManager::getSingleton()._requestSoundSource(this) )
				return;

		alGetError();
		// Play source
		alSourcePlay(mSource);
		if ( alGetError() )
		{
			Ogre::LogManager::getSingleton().logMessage("Unable to play sound");
			return;
		}
		// Set play flag
		mPlay = true;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamSound::_stopImpl()
	{
		if(mSource != AL_NONE)
		{
			// Remove audio data from source
			_dequeue();

			// Stop playback
			mPlay = false;

			// Jump to beginning if seeking available
			if ( mSeekable ) 
			{
				ov_time_seek(&mOggStream,0);
			}
			// Non-seekable - close/reopen
			else
			{
				// Close
				_release();

				// Reopen
				_openImpl(mAudioStream);
			}

			// Reload data
			_prebuffer();

			// Give up source immediately if specfied
			if (mGiveUpSource) OgreOggSoundManager::getSingleton()._releaseSoundSource(this);
		}
	}
}
