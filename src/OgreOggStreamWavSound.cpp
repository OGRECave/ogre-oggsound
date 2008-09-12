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

#include "OgreOggStreamWavSound.h"
#include <string>
#include <iostream>
#include "OgreOggSoundManager.h"

using namespace std;

namespace OgreOggSound
{

	/*/////////////////////////////////////////////////////////////////*/
	OgreOggStreamWavSound::OgreOggStreamWavSound(const Ogre::String& name) : OgreOggISound(name)
	{
		mStream=true;
		for ( int i=0; i<NUM_BUFFERS; i++ ) mBuffers[i]=0;		
		mStreamEOF=false;	
		mFormatData=0;
	}
	/*/////////////////////////////////////////////////////////////////*/
	OgreOggStreamWavSound::~OgreOggStreamWavSound()
	{
		_release();
		for ( int i=0; i<NUM_BUFFERS; i++ ) mBuffers[i]=0;		
		if (mFormatData) delete mFormatData;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamWavSound::open(Ogre::DataStreamPtr& fileStream)
	{
		// WAVE descriptor vars
		char	id[5]={0}; 
		short	format_tag;
		DWORD	size; 

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
				// Create format struct
				if (!mFormatData) mFormatData = new WavFormatData;

				// Read in "fmt" id ( 4 bytes ) 
				mAudioStream->read(id, 4);					

				// Read in "fmt" chunk size ( 4 bytes ) 
				mAudioStream->read(&mFormatData->mFormatChunkSize, 4);

				// Should be 16 unless compressed ( compressed NOT supported )
				if ( mFormatData->mFormatChunkSize==16 )
				{
					// Read in audio format  ( 2 bytes ) 
					mAudioStream->read(&format_tag, 2);		

					// Read in num channels ( 2 bytes ) 
					mAudioStream->read(&mFormatData->mNumChannels, 2);			

					// Read in sample rate ( 4 bytes ) 
					mAudioStream->read(&mFormatData->mSampleRate, 4);		

					// Read in byte rate ( 4 bytes ) 
					mAudioStream->read(&mFormatData->mAvgBytesPerSec, 4);	

					// Read in byte align ( 2 bytes ) 
					mAudioStream->read(&mFormatData->mBlockAlign, 2);		

					// Read in bits per sample ( 2 bytes ) 
					mAudioStream->read(&mFormatData->mBitsPerSample, 2);	

					// Read in "data" chunk id ( 4 bytes ) 
					mAudioStream->read(id, 4);					

					// Check for 'data ' or 'fact ' chunk
					if ( !strcmp(id, "data") )
					{
						// Read in size of audio data ( 4 bytes ) 
						mAudioStream->read(&mFormatData->mDataSize, 4);		

						// Store byte offset of start of audio data
						mFormatData->mAudioOffset = static_cast<DWORD>(mAudioStream->tell());
					}
					else
					{
						Ogre::LogManager::getSingleton().logMessage("*** --- OgreOggStreamWavSound::open() - No 'data' chunk!!", Ogre::LML_CRITICAL);
						throw std::string("WAVE load fail!");
					}
				}
				else
				{
					Ogre::LogManager::getSingleton().logMessage("*** --- OgreOggStreamWavSound::open() - Compressed WAVE files not supported!!", Ogre::LML_CRITICAL);
					throw std::string("WAVE load fail!");
				}
			}
			else
			{
				Ogre::LogManager::getSingleton().logMessage("*** --- OgreOggStreamWavSound::open() - Not a valid WAVE file!!", Ogre::LML_CRITICAL);
				throw std::string("WAVE load fail!");
			}
		}
		else
		{
			Ogre::LogManager::getSingleton().logMessage("*** --- OgreOggStreamWavSound::open() - Not a vlid RIFF file!!", Ogre::LML_CRITICAL);
			throw std::string("WAVE load fail!");
		}

		// Generate audio buffers
		alGenBuffers(NUM_BUFFERS, mBuffers);

		// Upload to XRAM buffers if available
		if ( OgreOggSoundManager::getSingleton().hasXRamSupport() )
			OgreOggSoundManager::getSingleton().setXRamBuffer(NUM_BUFFERS, mBuffers);

		// Work out required buffer size and format
		_calculateBufferInfo();

	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamWavSound::_calculateBufferInfo()
	{
		if (!mFormatData) return;

		switch(mFormatData->mNumChannels)
		{
		case 1:
			{
				if ( mFormatData->mBitsPerSample==8 )
				{
					// 8-bit mono
					mFormat = AL_FORMAT_MONO8;

					// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
					mBufferSize = mFormatData->mSampleRate/4;
				}
				else
				{
					// 16-bit mono
					mFormat = AL_FORMAT_MONO16;

					// Queue 250ms of audio data
					mBufferSize = mFormatData->mAvgBytesPerSec >> 2;
		
					// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
					mBufferSize -= (mBufferSize % mFormatData->mBlockAlign);
				}
			}
			break;
		case 2:
			{
				if ( mFormatData->mBitsPerSample==8 )
				{
					// 8-bit stereo
					mFormat = AL_FORMAT_STEREO8;

					// Set BufferSize to 250ms (Frequency * 2 (8bit stereo) divided by 4 (quarter of a second))
					mBufferSize = mFormatData->mSampleRate >> 1;

					// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
					mBufferSize -= (mBufferSize % 2);
				}
				else
				{
					// 16-bit stereo
					mFormat = AL_FORMAT_STEREO16;

					// Queue 250ms of audio data
					mBufferSize = mFormatData->mAvgBytesPerSec >> 2;

					// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
					mBufferSize -= (mBufferSize % mFormatData->mBlockAlign);
				}
			}
			break;
		case 6:
			{
				// 16-bit 5.1 surround
				mFormat = alGetEnumValue("AL_FORMAT_51CHN16");

				// Queue 250ms of audio data
				mBufferSize = mFormatData->mAvgBytesPerSec >> 2;

				// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
				mBufferSize -= (mBufferSize % mFormatData->mBlockAlign);
			}
			break;
		default:
			{
				// Error message
				Ogre::LogManager::getSingleton().logMessage("*** --- Unable to determine number of channels: defaulting to 16-bit mono");

				// 16-bit stereo
				mFormat = AL_FORMAT_STEREO16;

				// Queue 250ms of audio data
				mBufferSize = mFormatData->mAvgBytesPerSec >> 2;

				// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
				mBufferSize -= (mBufferSize % mFormatData->mBlockAlign);
			}
			break;
		}
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamWavSound::_release()
	{		
		ALuint src=AL_NONE;
		setSource(src);
		alDeleteBuffers(NUM_BUFFERS, mBuffers);
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamWavSound::_prebuffer()
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
	void OgreOggStreamWavSound::setSource(ALuint& src)
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
	void OgreOggStreamWavSound::_updateAudioBuffers()
	{	
		if(mSource == AL_NONE || !mPlay) return;	

		ALenum state;    
		alGetSourcei(mSource, AL_SOURCE_STATE, &state);	

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
	}
	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggStreamWavSound::_stream(ALuint buffer)
	{
		std::vector<char> audioData;
		char* data;		
		int  bytes = 0;
		int  result = 0;	

		// Create buffer
		data = new char[mBufferSize];

		// Read only what was asked for
		while(static_cast<int>(audioData.size()) < mBufferSize)
		{
			// Read up to a buffer's worth of data
			bytes = static_cast<int>(mAudioStream->read(data, mBufferSize));
			// EOF check
			if (mAudioStream->eof()) 
			{
				// If set to loop wrap to start of stream
				if ( mLoop )
				{
					mAudioStream->seek(mFormatData->mAudioOffset);
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
					// EOF - finish.
					if (bytes==0) break;
				}
			}
			// Append to end of buffer
			audioData.insert(audioData.end(), data, data + bytes);
			// Keep track of read data
			result+=bytes;
		}

		// EOF
		if(result == 0)	return false;

		alGetError();
		// Copy buffer data
		alBufferData(buffer, mFormat, &audioData[0], static_cast<ALsizei>(audioData.size()), mFormatData->mSampleRate);

		// Cleanup
		delete [] data;

		return true;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamWavSound::_dequeue()
	{		
		if(mSource == AL_NONE)
			return;
		
		int queued=0;

		alGetError();

		// Stop source to allow unqueuing
		alSourceStop(mSource);

		// Get number of buffers queued on source
		alGetSourcei(mSource, AL_BUFFERS_PROCESSED, &queued);

		// Remove number of buffers from source
		if (queued) 
		{
			alSourceUnqueueBuffers(mSource, queued, mBuffers);

			// Any problems?
			if ( alGetError() ) Ogre::LogManager::getSingleton().logMessage("*** Unable to unqueue buffers");
		}
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamWavSound::pause()
	{		
		if(mSource != AL_NONE)
		{
			alSourcePause(mSource);
		}
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamWavSound::play()
	{	
		if(isPlaying())	return;

		// Grab a source if not already attached
		if (mSource == AL_NONE)
			if ( !OgreOggSoundManager::getSingleton().requestSoundSource(this) )
				return;
	
		// Set play flag
		mPlay = true;

		// Play source
		alSourcePlay(mSource);	
		if ( alGetError() ) Ogre::LogManager::getSingleton().logMessage("Unable to play sound");
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamWavSound::stop()
	{
		if(mSource != AL_NONE)
		{
			// Remove audio data from source
			_dequeue();

			// Stop playback
			mPlay=false;

			// Reset stream pointer
			mAudioStream->seek(mFormatData->mAudioOffset);

			// Reload audio data
			_prebuffer();

			// Give up source immediately if specfied
			if (mGiveUpSource) OgreOggSoundManager::getSingleton().releaseSoundSource(this);
		}
	}
}