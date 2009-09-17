/**
* @file OgreOggStreamWavSound.cpp
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

#include "OgreOggStreamWavSound.h"
#include <string>
#include <iostream>
#include "OgreOggSoundManager.h"

using namespace std;

namespace OgreOggSound
{

	/*/////////////////////////////////////////////////////////////////*/
	OgreOggStreamWavSound::OgreOggStreamWavSound(const Ogre::String& name) : OgreOggISound(name)
	, mStreamEOF(false)
	{
		for ( int i=0; i<NUM_BUFFERS; i++ ) mBuffers[i]=AL_NONE;
		mStream = true;	   
	}
	/*/////////////////////////////////////////////////////////////////*/
	OgreOggStreamWavSound::~OgreOggStreamWavSound()
	{
		_release();
		for ( int i=0; i<NUM_BUFFERS; i++ ) mBuffers[i]=0;
		if (mFormatData.mFormat) OGRE_FREE(mFormatData.mFormat, Ogre::MEMCATEGORY_GENERAL);
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamWavSound::_openImpl(Ogre::DataStreamPtr& fileStream)
	{
		// WAVE descriptor vars
		char*			sound_buffer=0;
		int				bytesRead=0;
		ChunkHeader		c;

		// Store stream pointer
		mAudioStream = fileStream;

		// Allocate format structure
		mFormatData.mFormat = OGRE_NEW_T(WaveHeader, Ogre::MEMCATEGORY_GENERAL);

		// Read in "RIFF" chunk descriptor (4 bytes)
		mAudioStream->read(mFormatData.mFormat, sizeof(WaveHeader));

		// Valid 'RIFF'?
		if ( mFormatData.mFormat->mRIFF[0]=='R' && mFormatData.mFormat->mRIFF[1]=='I' && mFormatData.mFormat->mRIFF[2]=='F' && mFormatData.mFormat->mRIFF[3]=='F' )
		{
			// Valid 'WAVE'?
			if ( mFormatData.mFormat->mWAVE[0]=='W' && mFormatData.mFormat->mWAVE[1]=='A' && mFormatData.mFormat->mWAVE[2]=='V' && mFormatData.mFormat->mWAVE[3]=='E' )
			{
				// Valid 'fmt '?
				if ( mFormatData.mFormat->mFMT[0]=='f' && mFormatData.mFormat->mFMT[1]=='m' && mFormatData.mFormat->mFMT[2]=='t' && mFormatData.mFormat->mFMT[3]==' ' )
				{
					// SmFormatData.mFormat->uld be 16 unless compressed ( compressed NOT supported )
					if ( mFormatData.mFormat->mHeaderSize>=16 )
					{
						// PCM == 1
						if (mFormatData.mFormat->mFormatTag==0x0001 || mFormatData.mFormat->mFormatTag==0xFFFE)
						{
							// Samples check..
							if ( (mFormatData.mFormat->mBitsPerSample!=16) && (mFormatData.mFormat->mBitsPerSample!=8) )
							{
								Ogre::LogManager::getSingleton().logMessage("*** --- OgreOggStaticWavSound::open() - WAV BitsPerSample not 8/16!!", Ogre::LML_CRITICAL);
								throw std::string("WAVE load fail!");
							}

							// Calculate extra WAV header info
							long int extraBytes = mFormatData.mFormat->mHeaderSize - (sizeof(WaveHeader) - 20);

							// If WAVEFORMATEXTENSIBLE read attributes
							if (mFormatData.mFormat->mFormatTag==0xFFFE)
							{
								extraBytes-=static_cast<long>(mAudioStream->read(&mFormatData.mSamples, 2));
								extraBytes-=static_cast<long>(mAudioStream->read(&mFormatData.mChannelMask, 2));
								extraBytes-=static_cast<long>(mAudioStream->read(&mFormatData.mSubFormat, 16));
							}
		
							// Skip
							mAudioStream->skip(extraBytes);

							do
							{
								// Read in chunk id ( 4 bytes )
								mAudioStream->read(&c, sizeof(ChunkHeader));

								// 'data' chunk...
								if ( c.chunkID[0]=='d' && c.chunkID[1]=='a' && c.chunkID[2]=='t' && c.chunkID[3]=='a' )
								{
									// Store byte offset of start of audio data
									mAudioOffset = static_cast<unsigned long>(mAudioStream->tell());

									// Allocate array
									sound_buffer = OGRE_ALLOC_T(char, c.length, Ogre::MEMCATEGORY_GENERAL);

									// Read entire sound data
									bytesRead = static_cast<int>(mAudioStream->read(sound_buffer, c.length));

									// Jump out
									break;
								}
								// Unsupported chunk...
								else
									mAudioStream->skip(c.length);
							}
							while ( mAudioStream->eof() || c.chunkID[0]!='d' || c.chunkID[1]!='a' || c.chunkID[2]!='t' || c.chunkID[3]!='a' );							
						}
					}
					else
					{
						Ogre::LogManager::getSingleton().logMessage("*** --- OgreOggStaticWavSound::open() - WAV not PCM!!", Ogre::LML_CRITICAL);
						throw std::string("WAVE load fail!");
					}
				}
				else
				{
					Ogre::LogManager::getSingleton().logMessage("*** --- OgreOggStaticWavSound::open() - Invalid format!!", Ogre::LML_CRITICAL);
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
			Ogre::LogManager::getSingleton().logMessage("*** --- OgreOggStaticWavSound::open() - Not a valid RIFF file!!", Ogre::LML_CRITICAL);
			throw std::string("WAVE load fail!");
		}

		// Create OpenAL buffer
		alGetError();
		alGenBuffers(NUM_BUFFERS, mBuffers);
		if ( alGetError()!=AL_NO_ERROR )
			throw std::string("Unable to create OpenAL buffers!");

		// Check format support
		if (!_queryBufferInfo())
		{
			Ogre::LogManager::getSingleton().logMessage("*** --- Format NOT supported");
			throw std::string("Format NOT supported!");
		}

		// Calculate length in seconds
		mPlayTime = ((mFormatData.mFormat->mLength-mAudioOffset) / ((mFormatData.mFormat->mBitsPerSample/8) * mFormatData.mFormat->mSamplesPerSec)) / mFormatData.mFormat->mChannels;

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
		// Upload to XRAM buffers if available
		if ( OgreOggSoundManager::getSingleton().hasXRamSupport() )
			OgreOggSoundManager::getSingleton().setXRamBuffer(NUM_BUFFERS, mBuffers);
#endif
	}
	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggStreamWavSound::_queryBufferInfo()
	{
		if ( !mFormatData.mFormat ) return false;

		switch(mFormatData.mFormat->mChannels)
		{
		case 1:
			{
				if ( mFormatData.mFormat->mBitsPerSample==8 )
				{
					// 8-bit mono
					mFormat = AL_FORMAT_MONO8;

					// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
					mBufferSize = mFormatData.mFormat->mSamplesPerSec/4;
				}
				else
				{
					// 16-bit mono
					mFormat = AL_FORMAT_MONO16;

					// Queue 250ms of audio data
					mBufferSize = mFormatData.mFormat->mAvgBytesPerSec >> 2;

					// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
					mBufferSize -= (mBufferSize % mFormatData.mFormat->mBlockAlign);
				}
			}
			break;
		case 2:
			{
				if ( mFormatData.mFormat->mBitsPerSample==8 )
				{
					// 8-bit stereo
					mFormat = AL_FORMAT_STEREO8;

					// Set BufferSize to 250ms (Frequency * 2 (8bit stereo) divided by 4 (quarter of a second))
					mBufferSize = mFormatData.mFormat->mSamplesPerSec >> 1;

					// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
					mBufferSize -= (mBufferSize % 2);
				}
				else
				{
					// 16-bit stereo
					mFormat = AL_FORMAT_STEREO16;

					// Queue 250ms of audio data
					mBufferSize = mFormatData.mFormat->mAvgBytesPerSec >> 2;

					// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
					mBufferSize -= (mBufferSize % mFormatData.mFormat->mBlockAlign);
				}
			}
			break;
		case 4:
			{
				// 16-bit Quad surround
				mFormat = alGetEnumValue("AL_FORMAT_QUAD16");
				if (!mFormat) return false;

				// Queue 250ms of audio data
				mBufferSize = mFormatData.mFormat->mAvgBytesPerSec >> 2;

				// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
				mBufferSize -= (mBufferSize % mFormatData.mFormat->mBlockAlign);
			}
			break;
		case 6:
			{
				// 16-bit 5.1 surround
				mFormat = alGetEnumValue("AL_FORMAT_51CHN16");
				if (!mFormat) return false;

				// Queue 250ms of audio data
				mBufferSize = mFormatData.mFormat->mAvgBytesPerSec >> 2;

				// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
				mBufferSize -= (mBufferSize % mFormatData.mFormat->mBlockAlign);
			}
			break;
		case 7:
			{
				// 16-bit 7.1 surround
				mFormat = alGetEnumValue("AL_FORMAT_71CHN16");
				if (!mFormat) return false;

				// Queue 250ms of audio data
				mBufferSize = mFormatData.mFormat->mAvgBytesPerSec >> 2;

				// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
				mBufferSize -= (mBufferSize % mFormatData.mFormat->mBlockAlign);
			}
			break;
		case 8:
			{
				// 16-bit 8.1 surround
				mFormat = alGetEnumValue("AL_FORMAT_81CHN16");
				if (!mFormat) return false;

				// Queue 250ms of audio data
				mBufferSize = mFormatData.mFormat->mAvgBytesPerSec >> 2;

				// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
				mBufferSize -= (mBufferSize % mFormatData.mFormat->mBlockAlign);
			}
			break;
		default:
			{
				// Error message
				Ogre::LogManager::getSingleton().logMessage("*** --- Unable to determine number of channels: defaulting to 16-bit stereo");

				// 16-bit stereo
				mFormat = AL_FORMAT_STEREO16;

				// Queue 250ms of audio data
				mBufferSize = mFormatData.mFormat->mAvgBytesPerSec >> 2;

				// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
				mBufferSize -= (mBufferSize % mFormatData.mFormat->mBlockAlign);
			}
			break;
		}
		return true;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamWavSound::_release()
	{
		if ( mSource!=AL_NONE )
		{
			ALuint src=AL_NONE;
			setSource(src);
		}
		for (int i=0; i<NUM_BUFFERS; i++)
		{
			if (mBuffers[i]!=AL_NONE)
				alDeleteBuffers(1, &mBuffers[i]);
		}
		mPlayPosChanged = false;
		mPlayPos = 0.f;
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
			if ( _stream(buffer) ) 
			{
				alSourceQueueBuffers(mSource, 1, &buffer);
			}
		}

		// Handle play position change
		if ( mPlayPosChanged )
		{
			_updatePlayPosition();
			mPlayPosChanged = false;
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
		data = OGRE_ALLOC_T(char, mBufferSize, Ogre::MEMCATEGORY_GENERAL);
		
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
					mAudioStream->seek(mAudioOffset);
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
		if(result == 0)
		{
			OGRE_FREE(data, Ogre::MEMCATEGORY_GENERAL);
			return false;
		}

		alGetError();
		// Copy buffer data
		alBufferData(buffer, mFormat, &audioData[0], static_cast<ALsizei>(audioData.size()), mFormatData.mFormat->mSamplesPerSec);

		// Cleanup
		OGRE_FREE(data, Ogre::MEMCATEGORY_GENERAL);

		return true;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamWavSound::_dequeue()
	{
		if(mSource == AL_NONE)
			return;

		int queued=0;

		alGetError();

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

		// Get number of buffers queued on source
		alGetSourcei(mSource, AL_BUFFERS_PROCESSED, &queued);

		// Remove number of buffers from source
		while (queued--)
		{
			ALuint buffer;
			alSourceUnqueueBuffers(mSource, 1, &buffer);

			// Any problems?
			if ( alGetError() ) 
			{
				Ogre::LogManager::getSingleton().logMessage("*** Unable to unqueue buffers");
			}
		}
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamWavSound::_pauseImpl()
	{
		if(mSource == AL_NONE) return;

		alSourcePause(mSource);
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamWavSound::_playImpl()
	{
		if(isPlaying())	return;

		// Grab a source if not already attached
		if (mSource == AL_NONE)
			if ( !OgreOggSoundManager::getSingleton()._requestSoundSource(this) )
				return;

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
	void OgreOggStreamWavSound::setPlayPosition(Ogre::Real seconds)
	{
		if(seconds < 0) return;

		// Wrap
		if ( seconds>mPlayTime ) 
			do { seconds-=mPlayTime; } while ( seconds>mPlayTime );

		// Store play position
		mPlayPos = seconds;

		// Set flag
		mPlayPosChanged = true;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamWavSound::_updatePlayPosition()
	{
		if ( mSource==AL_NONE ) 
			return;

		// Get state
		bool playing = isPlaying();
		bool paused = isPaused();

		// Stop playback
		pause();

		// mBufferSize is 1/4 of a second
		size_t dataOffset = static_cast<size_t>(mPlayPos * mBufferSize * 4);
		mAudioStream->seek(mAudioOffset + dataOffset);

		// Unqueue audio
		_dequeue();

		// Fill buffers
		_prebuffer();

		// Set state
		if		(playing) play();
		else if	(paused) pause();
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamWavSound::_stopImpl()
	{
		if(mSource != AL_NONE)
		{
			// Remove audio data from source
			_dequeue();

			// Stop playback
			mPlay=false;

			// Reset stream pointer
			mAudioStream->seek(mAudioOffset);

			// Reload audio data
			_prebuffer();

			// Give up source immediately if specfied
			if (mGiveUpSource) OgreOggSoundManager::getSingleton()._releaseSoundSource(this);
		}
	}
}
