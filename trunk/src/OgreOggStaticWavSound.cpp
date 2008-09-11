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

#include "OgreOggStaticWavSound.h"
#include <string>
#include <iostream>
#include "OgreOggSoundManager.h"

using namespace std;

namespace OgreOggSound
{

	/*/////////////////////////////////////////////////////////////////*/
	OgreOggStaticWavSound::OgreOggStaticWavSound(const Ogre::String& name) : OgreOggISound(name)
	{
		mStream=false;
		mOggFile=0;						
		mVorbisInfo=0;			
		mVorbisComment=0;		
		mBufferData.clear();	
		mPreviousOffset=0;
		mBuffer=0;						
	}
	/*/////////////////////////////////////////////////////////////////*/
	OgreOggStaticWavSound::~OgreOggStaticWavSound()
	{
		_release();
		mOggFile=0;						
		mVorbisInfo=0;			
		mVorbisComment=0;		
		mBufferData.clear();		
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStaticWavSound::open(Ogre::DataStreamPtr& fileStream)
	{
		// WAVE descriptor vars
		char*	sound_buffer=0;
		char	id[5]={0}; 
		short	format_tag;
		DWORD	size; 
		int		bytesRead=0;

		// Store stream pointer
		mAudioStream = fileStream;

		// Read in "RIFF" chunk descriptor (4 bytes)
		mAudioStream->read(id, 4); 

		// Valid RIFF?
		if (!strcmp(id, "RIFF"))
		{ 
			// Read in chunk size (4 bytes)
			mAudioStream->read(&size, 4);					

			// Read in "WAVE" format descriptor (4 bytes)
			mAudioStream->read(id, 4);						

			// Valid wav?
			if (!strcmp(id,"WAVE"))
			{ 
				// Read in "fmt" id ( 4 bytes ) 
				mAudioStream->read(id, 4);					

				// Read in "fmt" chunk size ( 4 bytes ) 
				mAudioStream->read(&mFormatData.mFormatChunkSize, 4);

				// Should be 16 unless compressed ( compressed NOT supported )
				if ( mFormatData.mFormatChunkSize==16 )
				{
					// Read in audio format  ( 2 bytes ) 
					mAudioStream->read(&format_tag, 2);		

					// Read in num channels ( 2 bytes ) 
					mAudioStream->read(&mFormatData.mNumChannels, 2);			

					// Read in sample rate ( 4 bytes ) 
					mAudioStream->read(&mFormatData.mSampleRate, 4);		

					// Read in byte rate ( 4 bytes ) 
					mAudioStream->read(&mFormatData.mAvgBytesPerSec, 4);	

					// Read in byte align ( 2 bytes ) 
					mAudioStream->read(&mFormatData.mBlockAlign, 2);		

					// Read in bits per sample ( 2 bytes ) 
					mAudioStream->read(&mFormatData.mBitsPerSample, 2);	

					// Read in "data" chunk id ( 4 bytes ) 
					mAudioStream->read(id, 4);					

					// Check for 'data ' or 'fact ' chunk
					if ( !strcmp(id, "data") )
					{
						// Read in size of audio data ( 4 bytes ) 
						mAudioStream->read(&mFormatData.mDataSize, 4);		

						// Store byte offset of start of audio data
						mFormatData.mAudioOffset = static_cast<DWORD>(mAudioStream->tell());

						// Allocate array
						sound_buffer = new char[mFormatData.mDataSize];

						// Read entire sound data
						bytesRead = static_cast<int>(mAudioStream->read(sound_buffer, mFormatData.mDataSize));
					}
					else
					{
						Ogre::LogManager::getSingleton().logMessage("*** --- OgreOggStaticWavSound::open() - No 'data' chunk!!", Ogre::LML_CRITICAL);
						throw std::string("WAVE load fail!");
					}
				}
				else
				{
					Ogre::LogManager::getSingleton().logMessage("*** --- OgreOggStaticWavSound::open() - Compressed WAVE files not supported!!", Ogre::LML_CRITICAL);
					throw std::string("WAVE load fail!");
				}
			}
			else
			{
				Ogre::LogManager::getSingleton().logMessage("*** --- OgreOggStaticWavSound::open() - Not a valid WAVE file!!", Ogre::LML_CRITICAL);
				throw std::string("WAVE load fail!");
			}
		}
		else
		{
			Ogre::LogManager::getSingleton().logMessage("*** --- OgreOggStaticWavSound::open() - Not a vlid RIFF file!!", Ogre::LML_CRITICAL);
			throw std::string("WAVE load fail!");
		}


		// Create OpenAL buffer
		alGetError();
		alGenBuffers(1, &mBuffer);
		if ( alGetError()!=AL_NO_ERROR )
			throw std::string("Unable to create OpenAL buffer!");

		// Upload to XRAM buffers if available
		if ( OgreOggSoundManager::getSingleton().hasXRamSupport() )
			OgreOggSoundManager::getSingleton().setXRamBuffer(1, &mBuffer);

		// Set format
		if(mFormatData.mNumChannels==1)
		{
			if(mFormatData.mBitsPerSample==16)
				mFormat = AL_FORMAT_MONO16;
			else
				mFormat = AL_FORMAT_MONO8;
		}
		else
		{
			if(mFormatData.mBitsPerSample==16)
				mFormat = AL_FORMAT_STEREO16;
			else
				mFormat = AL_FORMAT_STEREO8;
		}

		alGetError();
		alBufferData(mBuffer, mFormat, sound_buffer, static_cast<ALsizei>(bytesRead-1), mFormatData.mSampleRate);
		if ( alGetError()!=AL_NO_ERROR )
		{
			Ogre::LogManager::getSingleton().logMessage("*** --- OgreOggStaticWavSound::open() - Unable to load audio data into buffer!!", Ogre::LML_CRITICAL);
			throw std::string("Unable to load buffers with data!");
		}
		delete [] sound_buffer;
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStaticWavSound::_release()
	{
		ALuint src=AL_NONE;
		setSource(src);
		alDeleteBuffers(1,&mBuffer);
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStaticWavSound::_prebuffer()
	{
		if (mSource==AL_NONE) return;

		// Queue buffer
		alSourcei(mSource, AL_BUFFER, mBuffer);
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStaticWavSound::setSource(ALuint& src)
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
	void OgreOggStaticWavSound::pause()
	{		
		if ( mSource!=AL_NONE ) return;

		alSourcePause(mSource);
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStaticWavSound::play()
	{	
		if(isPlaying())
			return;

		if (mSource == AL_NONE)
			if ( !OgreOggSoundManager::getSingleton().requestSoundSource(this) )
				return;

		alSourcePlay(mSource);	
		mPlay=true;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStaticWavSound::stop()
	{
		if ( mSource==AL_NONE ) return;

		alSourceStop(mSource);
		alSourceRewind(mSource);	
		mPlay=false;
		mPreviousOffset=0;

		// Give up source immediately if specfied
		if (mGiveUpSource) OgreOggSoundManager::getSingleton().releaseSoundSource(this);
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStaticWavSound::loop(bool loop)
	{
		OgreOggISound::loop(loop);

		if(mSource != AL_NONE)
		{
			alSourcei(mSource,AL_LOOPING, loop);
		}
	}	
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStaticWavSound::_updateAudioBuffers()
	{
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