/**
* @file OgreOggStaticWavSound.cpp
* @author  Ian Stangoe
* @version 1.13
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

#include "OgreOggStaticWavSound.h"
#include <string>
#include <iostream>
#include "OgreOggSoundManager.h"

using namespace std;

namespace OgreOggSound
{

	/*/////////////////////////////////////////////////////////////////*/
			OgreOggStaticWavSound::OgreOggStaticWavSound(const Ogre::String& name) : OgreOggISound(name)
		,mAudioName("")
		,mPreviousOffset(0)
		,mBuffer(0)
		{
			mStream=false;
			mFormatData.mFormat=0;
			mBufferData.clear();
		}
	/*/////////////////////////////////////////////////////////////////*/
			OgreOggStaticWavSound::~OgreOggStaticWavSound()
	{
		_release();
		mBufferData.clear();
		if (mFormatData.mFormat) OGRE_FREE(mFormatData.mFormat, Ogre::MEMCATEGORY_GENERAL);
	}
	/*/////////////////////////////////////////////////////////////////*/
	void	OgreOggStaticWavSound::_openImpl(Ogre::DataStreamPtr& fileStream)
	{
		// WAVE descriptor vars
		char*			sound_buffer=0;
		int				bytesRead=0;
		ChunkHeader		c;

		// Store stream pointer
		mAudioStream = fileStream;

		// Store file name
		mAudioName = mAudioStream->getName();

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
							unsigned long int extraBytes = mFormatData.mFormat->mHeaderSize - (sizeof(WaveHeader) - 20);

							// If WAVEFORMATEXTENSIBLE read attributes
							if (mFormatData.mFormat->mFormatTag==0xFFFE)
							{
								extraBytes-=static_cast<unsigned long int>(mAudioStream->read(&mFormatData.mSamples, 2));
								extraBytes-=static_cast<unsigned long int>(mAudioStream->read(&mFormatData.mChannelMask, 2));
								extraBytes-=static_cast<unsigned long int>(mAudioStream->read(&mFormatData.mSubFormat, 16));
							}
		
							// Skip
							mAudioStream->skip(extraBytes);

							do
							{
								// Read in chunk header
								mAudioStream->read(&c, sizeof(ChunkHeader));

								// 'data' chunk...
								if ( c.chunkID[0]=='d' && c.chunkID[1]=='a' && c.chunkID[2]=='t' && c.chunkID[3]=='a' )
								{
									// Store byte offset of start of audio data
									mAudioOffset = static_cast<unsigned long>(mAudioStream->tell());

									// Check data size
									int fileCheck = c.length % mFormatData.mFormat->mBlockAlign;

									// Store end pos
									mAudioEnd = mAudioOffset+(c.length-fileCheck);

									// Allocate array
									sound_buffer = OGRE_ALLOC_T(char, mAudioEnd-mAudioOffset, Ogre::MEMCATEGORY_GENERAL);

									// Read entire sound data
									bytesRead = static_cast<int>(mAudioStream->read(sound_buffer, mAudioEnd-mAudioOffset));

									// Jump out
									break;
								}
								// Unsupported chunk...
								else
									mAudioStream->skip(c.length);
							}
							while ( mAudioStream->eof() || c.chunkID[0]!='d' || c.chunkID[1]!='a' || c.chunkID[2]!='t' || c.chunkID[3]!='a' );							
						}
						else 
						{
							Ogre::LogManager::getSingleton().logMessage("*** --- OgreOggStaticWavSound::open() - Compressed WAV NOT supported!!", Ogre::LML_CRITICAL);
							throw std::string("WAVE load fail!");
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
		alGenBuffers(1, &mBuffer);
		if ( alGetError()!=AL_NO_ERROR )
			throw std::string("Unable to create OpenAL buffer!");

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
		// Upload to XRAM buffers if available
		if ( OgreOggSoundManager::getSingleton().hasXRamSupport() )
			OgreOggSoundManager::getSingleton().setXRamBuffer(1, &mBuffer);
#endif
		// Check format support
		if (!_queryBufferInfo())
			throw std::string("Format NOT supported!");

		// Calculate length in seconds
		mPlayTime = (mAudioEnd-mAudioOffset) / (mFormatData.mFormat->mSamplesPerSec / mFormatData.mFormat->mChannels);

		alGetError();
		alBufferData(mBuffer, mFormat, sound_buffer, static_cast<ALsizei>(bytesRead), mFormatData.mFormat->mSamplesPerSec);
		if ( alGetError()!=AL_NO_ERROR )
		{
			Ogre::LogManager::getSingleton().logMessage("*** --- OgreOggStaticWavSound::open() - Unable to load audio data into buffer!!", Ogre::LML_CRITICAL);
			throw std::string("Unable to load buffers with data!");
		}
		OGRE_FREE(sound_buffer, Ogre::MEMCATEGORY_GENERAL);

		// Register shared buffer
		OgreOggSoundManager::getSingleton()._registerSharedBuffer(mAudioName, mBuffer);
	}

	/*/////////////////////////////////////////////////////////////////*/
	void	OgreOggStaticWavSound::_openImpl(const Ogre::String& fName, ALuint& buffer)
	{
		// Set buffer
		mBuffer = buffer;

		// Filename
		mAudioName = fName;
	}

	/*/////////////////////////////////////////////////////////////////*/
	bool	OgreOggStaticWavSound::_queryBufferInfo()
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
	void	OgreOggStaticWavSound::_release()
	{
		ALuint src=AL_NONE;
		setSource(src);
		OgreOggSoundManager::getSingleton()._releaseSharedBuffer(mAudioName, mBuffer);
		mPlayPosChanged = false;
		mPlayPos = 0.f;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void	OgreOggStaticWavSound::_prebuffer()
	{
		if (mSource==AL_NONE) return;

		// Queue buffer
		alSourcei(mSource, AL_BUFFER, mBuffer);
	}

	/*/////////////////////////////////////////////////////////////////*/
	void	OgreOggStaticWavSound::setSource(ALuint& src)
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
	void	OgreOggStaticWavSound::_pauseImpl()
	{
		if ( mSource==AL_NONE ) return;

		alSourcePause(mSource);
	}
	/*/////////////////////////////////////////////////////////////////*/
	void	OgreOggStaticWavSound::_playImpl()
	{
		if(isPlaying())
			return;

		if (mSource == AL_NONE)
			if ( !OgreOggSoundManager::getSingleton()._requestSoundSource(this) )
				return;

		// Pick up position change
		if ( mPlayPosChanged )
			setPlayPosition(mPlayPos);

		alSourcePlay(mSource);
		mPlay = true;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void	OgreOggStaticWavSound::_stopImpl()
	{
		if ( mSource==AL_NONE ) return;

		alSourceStop(mSource);
		alSourceRewind(mSource);
		mPlay=false;
		mPreviousOffset=0;

		if (mTemporary)
		{
			OgreOggSoundManager::getSingleton()._destroyTemporarySound(this);
		}
		// Give up source immediately if specfied
		else if (mGiveUpSource) 
			OgreOggSoundManager::getSingleton()._releaseSoundSource(this);
	}
	/*/////////////////////////////////////////////////////////////////*/
	void	OgreOggStaticWavSound::loop(bool loop)
	{
		OgreOggISound::loop(loop);

		if(mSource != AL_NONE)
		{
			alSourcei(mSource,AL_LOOPING, loop);
		}
	}
	/*/////////////////////////////////////////////////////////////////*/
	void	OgreOggStaticWavSound::_updateAudioBuffers()
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
