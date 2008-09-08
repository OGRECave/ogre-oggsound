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

#include "OgreOggStreamSound.h"
#include <string>
#include <iostream>
#include "OgreOggSoundManager.h"

using namespace std;

namespace OgreOggSound
{

	/*/////////////////////////////////////////////////////////////////*/
	OgreOggStreamSound::OgreOggStreamSound(const Ogre::String& name) : OgreOggISound(name)
	{
		mStream=true;
		mOggFile=0;						
		mVorbisInfo=0;			
		mVorbisComment=0;		
		for ( int i=0; i<NUM_BUFFERS; i++ ) mBuffers[i]=0;		
		mStreamEOF=false;	
	}
	/*/////////////////////////////////////////////////////////////////*/
	OgreOggStreamSound::~OgreOggStreamSound()
	{
		_release();
		mOggFile=0;						
		mVorbisInfo=0;			
		mVorbisComment=0;		
		for ( int i=0; i<NUM_BUFFERS; i++ ) mBuffers[i]=0;		
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamSound::open(Ogre::DataStreamPtr& fileStream)
	{
		int result;

		mOgreOggStream = fileStream;

		if((result = ov_open_callbacks(&mOgreOggStream, &mOggStream, NULL, 0, mOggCallbacks)) < 0)
		{
			throw string("Could not open Ogg stream. ");
			return;
		}

		mVorbisInfo = ov_info(&mOggStream, -1);
		mVorbisComment = ov_comment(&mOggStream, -1);

		if(mVorbisInfo->channels == 1)
			mFormat = AL_FORMAT_MONO16;
		else
			mFormat = AL_FORMAT_STEREO16;	

		alGenBuffers(NUM_BUFFERS, mBuffers);
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamSound::_release()
	{		
		ALuint src=AL_NONE;
		setSource(src);
		alDeleteBuffers(NUM_BUFFERS,mBuffers);
		ov_clear(&mOggStream);
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamSound::prebuffer()
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
	void OgreOggStreamSound::updateAudioBuffers()
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
					mFinishedCB->execute(dynamic_cast<OgreOggISound*>(this));
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
	bool OgreOggStreamSound::_stream(ALuint buffer)
	{
		std::vector<char> audioData;
		char data[BUFFER_SIZE];		
		int  bytes = 0;
		int  section = 0;
		int  result = 0;	

		// Read only what was asked for
		while(static_cast<int>(audioData.size()) < BUFFER_SIZE)
		{
			// Read up to a buffer's worth of data
			bytes = ov_read(&mOggStream, data, BUFFER_SIZE, 0, 2, 1, &section);
			// EOF check
			if (bytes == 0) 
			{
				// If set to loop wrap to start of stream
				if ( mLoop )
				{
					ov_time_seek(&mOggStream, 0);
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
		if(result == 0)	return false;

		alGetError();
		// Copy buffer data
		alBufferData(buffer, mFormat, &audioData[0], static_cast<ALsizei>(audioData.size()), mVorbisInfo->rate);

		return true;
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamSound::_check()
	{
		int error = alGetError();

		if(error != AL_NO_ERROR)
			throw string("OpenAL error was raised.");
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamSound::_dequeue()
	{		
		if(mSource == AL_NONE)
			return;
		
		int queued=0;

		// Stop source to allow unqueuing
		alSourceStop(mSource);

		// Get number of buffers queued on source
		alGetSourcei(mSource, AL_BUFFERS_QUEUED, &queued);

		// Remove number of buffers from source
		alSourceUnqueueBuffers(mSource, queued, mBuffers);

		// Any problems?
		if ( alGetError() ) Ogre::LogManager::getSingleton().logMessage("*** Unable to unqueue buffers");
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamSound::pause()
	{		
		if(mSource != AL_NONE)
		{
			alSourcePause(mSource);
		}
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamSound::play()
	{	
		if(isPlaying())	return;

		// Grab a source if not already attached
		if (mSource == AL_NONE)
			if ( OgreOggSoundManager::getSingleton().requestSoundSource(this) )
			{
				// Fill data buffers
				prebuffer();

				// Init source
				_initSource();
			}

		// Set play flag
		mPlay = true;

		// Play source
		alSourcePlay(mSource);	
		if ( alGetError() ) Ogre::LogManager::getSingleton().logMessage("Unable to play sound");
	}
	/*/////////////////////////////////////////////////////////////////*/
	void OgreOggStreamSound::stop()
	{
		if(mSource != AL_NONE)
		{
			alSourceStop(mSource);
			alSourceRewind(mSource);
			ov_time_seek(&mOggStream,0);	
			mPlay=false;
		}
	}
}