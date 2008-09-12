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

#include "OgreOggStaticSound.h"
#include <string>
#include <iostream>
#include "OgreOggSoundManager.h"

using namespace std;

namespace OgreOggSound
{

	void Load_Wave_File(const char *fname)
	{
		FILE *fp;

		fp = fopen(fname,"rb");
		if (fp)
		{
			char id[4], *sound_buffer; //four bytes to hold 'RIFF'

			DWORD size; //32 bit value to hold file size

			short	format_tag, 
				channels, 
				block_align, 
				bits_per_sample; //our 16 values

			DWORD	format_length, 
				sample_rate, 
				avg_bytes_sec, 
				data_size;

			fread(id, sizeof(BYTE), 4, fp); //read in first four bytes

			if (!strcmp(id, "RIFF"))
			{ 
				//we had 'RIFF' let's continue
				fread(&size, sizeof(DWORD), 1, fp); //read in 32bit size value
				fread(&id, sizeof(BYTE), 4, fp); //read in 4 byte string now

				if (!strcmp(id,"WAVE"))
				{ 
					//this is probably a wave file since it contained "WAVE"
					fread(id, sizeof(BYTE), 4, fp); //read in 4 bytes "fmt ";
					fread(&format_length, sizeof(DWORD),1,fp);
					fread(&format_tag, sizeof(short), 1, fp); //check mmreg.h (i think?) for other 
					// possible format tags like ADPCM
					fread(&channels, sizeof(short),1,fp); //1 mono, 2 stereo
					fread(&sample_rate, sizeof(DWORD), 1, fp); //like 44100, 22050, etc...
					fread(&avg_bytes_sec, sizeof(short), 1, fp); //probably won't need this
					fread(&block_align, sizeof(short), 1, fp); //probably won't need this
					fread(&bits_per_sample, sizeof(short), 1, fp); //8 bit or 16 bit file?
					fread(id, sizeof(BYTE), 4, fp); //read in 'data'
					fread(&data_size, sizeof(DWORD), 1, fp); //how many bytes of sound data we have
					sound_buffer = new char[data_size]; //set aside sound buffer space
					fread(sound_buffer, sizeof(BYTE), data_size, fp); //read in our whole sound data chunk
				}
				else
					printf("Error: RIFF file but not a wave file\n");
			}
			else
				printf("Error: not a RIFF file\n");
		}
	}

	/*/////////////////////////////////////////////////////////////////*/
	OgreOggStaticSound::OgreOggStaticSound(const Ogre::String& name) : OgreOggISound(name)
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
	OgreOggStaticSound::~OgreOggStaticSound()
	{
		_release();
		mOggFile=0;						
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

		if((result = ov_open_callbacks(&mAudioStream, &mOggStream, NULL, 0, mOggCallbacks)) < 0)
		{
			throw string("Could not open Ogg stream. ");
			return;
		}

		mVorbisInfo = ov_info(&mOggStream, -1);
		mVorbisComment = ov_comment(&mOggStream, -1);

		_calculateBufferInfo();

		alGenBuffers(1, &mBuffer);

		char* data;
		int sizeRead = 0;
		int bitStream;

		data = new char[mBufferSize];
		do
		{
			sizeRead = ov_read(&mOggStream, data, static_cast<int>(mBufferSize), 0, 2, 1, &bitStream);
			mBufferData.insert(mBufferData.end(), data, data + sizeRead);
		} 
		while(sizeRead > 0);
		delete [] data;

		// Upload to XRAM buffers if available
		if ( OgreOggSoundManager::getSingleton().hasXRamSupport() )
			OgreOggSoundManager::getSingleton().setXRamBuffer(1, &mBuffer);

		alGetError();
		alBufferData(mBuffer, mFormat, &mBufferData[0], static_cast < ALsizei > (mBufferData.size()), mVorbisInfo->rate);
		if ( alGetError()!=AL_NO_ERROR )
		{
			Ogre::LogManager::getSingleton().logMessage("*** --- OgreOggStaticSound::open() - Unable to load audio data into buffer!!", Ogre::LML_CRITICAL);
			throw std::string("Unable to load buffers with data!");
		}
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStaticSound::_release()
	{
		ALuint src=AL_NONE;
		setSource(src);
		alDeleteBuffers(1,&mBuffer);
		ov_clear(&mOggStream);
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStaticSound::_prebuffer()
	{
		if (mSource==AL_NONE) return;

		// Queue buffer
		alSourcei(mSource, AL_BUFFER, mBuffer);
	}

	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStaticSound::_calculateBufferInfo()
	{
		if (!mVorbisInfo) return;

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
				// Set BufferSize to 250ms (Frequency * 8 (16bit 4-channel) divided by 4 (quarter of a second))
				mBufferSize = mVorbisInfo->rate * 2;
				// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
				mBufferSize -= (mBufferSize % 8);
			}
			break;
		case 6:
			{
				mFormat = alGetEnumValue("AL_FORMAT_51CHN16");
				// Set BufferSize to 250ms (Frequency * 12 (16bit 6-channel) divided by 4 (quarter of a second))
				mBufferSize = mVorbisInfo->rate * 3;
				// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
				mBufferSize -= (mBufferSize % 12);
			}
			break;
		case 7:
			{
				mFormat = alGetEnumValue("AL_FORMAT_61CHN16");
				// Set BufferSize to 250ms (Frequency * 16 (16bit 7-channel) divided by 4 (quarter of a second))
				mBufferSize = mVorbisInfo->rate * 4;
				// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
				mBufferSize -= (mBufferSize % 16);
			}
			break;
		case 8:
			{
				mFormat = alGetEnumValue("AL_FORMAT_71CHN16");
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
		if ( mSource!=AL_NONE ) return;

		alSourcePause(mSource);
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStaticSound::play()
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
	void OgreOggStaticSound::stop()
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