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
#include "mmreg.h"

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
		mFormatData=0;
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
		if (mFormatData) delete mFormatData;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStaticWavSound::open(Ogre::DataStreamPtr& fileStream)
	{
		// WAVE descriptor vars
		char*	sound_buffer=0;
		char	id[5]={0}; 
		WORD	format_tag;
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
				// Create format struct
				if (!mFormatData) mFormatData = new WavFormatData;

				// Read in "fmt" id ( 4 bytes ) 
				mAudioStream->read(id, 4);					

				// Read in "fmt" chunk size ( 4 bytes ) 
				mAudioStream->read(&mFormatData->mFormatChunkSize, 4);

				// Should be 16 unless compressed ( compressed NOT supported )
				if ( mFormatData->mFormatChunkSize>=16 )
				{
					// Read in audio format  ( 2 bytes ) 
					mAudioStream->read(&format_tag, 2);		

					// PCM == 1
					if (format_tag==0x0001 || format_tag==0xFFFE)
					{
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

						// If WAVEFORMATEXTENSIBLE...
						if (format_tag==0xFFFE)
						{
							WORD sigBitsPerSample;
							WORD extraInfoSize;

							// Read in significant bits per sample ( 2 bytes ) 
							mAudioStream->read(&sigBitsPerSample, 2);	
							
							// Read in extra information size ( 2 bytes ) 
							mAudioStream->read(&extraInfoSize, 2);	

							// Read in samples ( 2 bytes ) 
							mAudioStream->read(&mFormatData->mSamples, 2);	

							// Read in channels mask ( 2 bytes ) 
							mAudioStream->read(&mFormatData->mChannelMask, 2);	
							
							// Read in sub format ( 16 bytes ) 
							mAudioStream->read(&mFormatData->mSubFormat, sizeof(GUID));	
						}


						// Read in chunk id ( 4 bytes ) 
						mAudioStream->read(id, 4);					

						if ( !strcmp(id, "data") )
						{
							try
							{
								// Read in size of audio data ( 4 bytes ) 
								mAudioStream->read(&mFormatData->mDataSize, 4);		

								// Store byte offset of start of audio data
								mFormatData->mAudioOffset = static_cast<DWORD>(mAudioStream->tell());

								// Allocate array
								sound_buffer = new char[mFormatData->mDataSize];

								// Read entire sound data
								bytesRead = static_cast<int>(mAudioStream->read(sound_buffer, mFormatData->mDataSize));
							}
							catch(...)
							{
								Ogre::LogManager::getSingleton().logMessage("*** --- OgreOggStaticWavSound::open() - error reading wav data!!", Ogre::LML_CRITICAL);
								throw std::string("WAVE load fail!");
							}
						}
						else
						{
							// Find "data" chunk
							try
							{
								do
								{
									DWORD chunkSize;

									// Read in size of chunk data ( 4 bytes ) 
									mAudioStream->read(&chunkSize, 4);		

									// Skip chunk
									mAudioStream->skip(chunkSize);

									// Read next chunk id
									mAudioStream->read(id, 4);
								}
								while ( strcmp(id, "data") || mAudioStream->eof() );

								// Validity check
								if (!mAudioStream->eof())
								{
									// Read in size of audio data ( 4 bytes ) 
									mAudioStream->read(&mFormatData->mDataSize, 4);		

									// Store byte offset of start of audio data
									mFormatData->mAudioOffset = static_cast<DWORD>(mAudioStream->tell());

									// Allocate array
									sound_buffer = new char[mFormatData->mDataSize];

									// Read entire sound data
									bytesRead = static_cast<int>(mAudioStream->read(sound_buffer, mFormatData->mDataSize));
								}
								else
								{
									Ogre::LogManager::getSingleton().logMessage("*** --- OgreOggStaticWavSound::open() - No wav data!!", Ogre::LML_CRITICAL);
									throw std::string("WAVE load fail!");
								}
							}
							catch(...)
							{
								Ogre::LogManager::getSingleton().logMessage("*** --- OgreOggStaticWavSound::open() - error reading wav data!!", Ogre::LML_CRITICAL);
								throw std::string("WAVE load fail!");
							}
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

		// Check format support
		if (!_queryBufferInfo()) 
			throw std::string("Format NOT supported!");

		alGetError();
		alBufferData(mBuffer, mFormat, sound_buffer, static_cast<ALsizei>(bytesRead), mFormatData->mSampleRate);
		if ( alGetError()!=AL_NO_ERROR )
		{
			Ogre::LogManager::getSingleton().logMessage("*** --- OgreOggStaticWavSound::open() - Unable to load audio data into buffer!!", Ogre::LML_CRITICAL);
			throw std::string("Unable to load buffers with data!");
		}
		delete [] sound_buffer;

		// Set ready flag
		mFileOpened = true;
	}

	/*/////////////////////////////////////////////////////////////////*/
	bool OgreOggStaticWavSound::_queryBufferInfo()
	{
		if (!mFormatData) return false;

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
		case 4:
			{
				// 16-bit Quad surround
				mFormat = alGetEnumValue("AL_FORMAT_QUAD16");
				if (!mFormat) return false; 

				// Queue 250ms of audio data
				mBufferSize = mFormatData->mAvgBytesPerSec >> 2;

				// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
				mBufferSize -= (mBufferSize % mFormatData->mBlockAlign);
			}
			break;
		case 6:
			{
				// 16-bit 5.1 surround
				mFormat = alGetEnumValue("AL_FORMAT_51CHN16");
				if (!mFormat) return false; 

				// Queue 250ms of audio data
				mBufferSize = mFormatData->mAvgBytesPerSec >> 2;

				// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
				mBufferSize -= (mBufferSize % mFormatData->mBlockAlign);
			}
			break;
		case 7:
			{
				// 16-bit 7.1 surround
				mFormat = alGetEnumValue("AL_FORMAT_71CHN16");
				if (!mFormat) return false; 

				// Queue 250ms of audio data
				mBufferSize = mFormatData->mAvgBytesPerSec >> 2;

				// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
				mBufferSize -= (mBufferSize % mFormatData->mBlockAlign);
			}
			break;
		case 8:
			{
				// 16-bit 8.1 surround
				mFormat = alGetEnumValue("AL_FORMAT_81CHN16");
				if (!mFormat) return false; 

				// Queue 250ms of audio data
				mBufferSize = mFormatData->mAvgBytesPerSec >> 2;

				// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
				mBufferSize -= (mBufferSize % mFormatData->mBlockAlign);
			}
			break;
		default:
			{
				// Error message
				Ogre::LogManager::getSingleton().logMessage("*** --- Unable to determine number of channels: defaulting to 16-bit stereo");

				// 16-bit stereo
				mFormat = AL_FORMAT_STEREO16;

				// Queue 250ms of audio data
				mBufferSize = mFormatData->mAvgBytesPerSec >> 2;

				// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
				mBufferSize -= (mBufferSize % mFormatData->mBlockAlign);
			}
			break;
		}
		return true;
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
		if ( mSource==AL_NONE ) return;

		alSourcePause(mSource);
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStaticWavSound::play()
	{	
		if(isPlaying())
			return;
	
		if (!mFileOpened)	
		{
			mPlayDelayed = true;
			mPlay = true;
			return;
		}

		if (mSource == AL_NONE)
			if ( !OgreOggSoundManager::getSingleton().requestSoundSource(this) )
				return;

		alSourcePlay(mSource);	
		mPlay = true;
		mPlayDelayed = false;
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
		// Automatically play if previously delayed
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