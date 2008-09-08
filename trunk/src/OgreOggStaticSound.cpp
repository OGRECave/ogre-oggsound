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

	/*/////////////////////////////////////////////////////////////////*/
	OgreOggStaticSound::OgreOggStaticSound(const Ogre::String& name) : OgreOggISound(name)
	{
		mStream=false;
		mOggFile=0;						
		mVorbisInfo=0;			
		mVorbisComment=0;		
		mBufferData.clear();		
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

		alGenBuffers(1, &mBuffer);


		char data[BUFFER_SIZE];
		int sizeRead = 0;
		int bitStream;
		do
		{
			sizeRead = ov_read(&mOggStream, data, BUFFER_SIZE, 0, 2, 1, &bitStream);
			mBufferData.insert(mBufferData.end(), data, data + sizeRead);
		} 
		while(sizeRead > 0);

		alBufferData(mBuffer, mFormat, &mBufferData[0], static_cast < ALsizei > (mBufferData.size()), mVorbisInfo->rate);
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
	void OgreOggStaticSound::setSource(ALuint& src)
	{
		if (src!=AL_NONE)
		{
			// Attach new source
			mSource=src;
			
			// Queue buffer
			alSourcei(mSource, AL_BUFFER, mBuffer);
			
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
	void OgreOggStaticSound::check()
	{
		int error = alGetError();

		if(error != AL_NO_ERROR)
			throw string("OpenAL error was raised.");
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
	void OgreOggStaticSound::updateAudioBuffers()
	{
		if(mSource == AL_NONE || !mPlay)
			return;

		ALenum state;    
		alGetSourcei(mSource, AL_SOURCE_STATE, &state);	

		if (state == AL_STOPPED)
		{
			stop();
			// Finished callback
			if ( mFinishedCB ) 
				mFinishedCB->execute(dynamic_cast<OgreOggISound*>(this));
		}
	}
}