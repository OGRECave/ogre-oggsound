/**
* @file OgreOggStaticSound.h
* @author  Ian Stangoe
* @version 1.14
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
* @section DESCRIPTION
* 
* Implements methods for creating/using a static ogg sound
*/

#pragma once

#include "OgreOggSoundPrereqs.h"
#include <string>
#include <vector>
#include "ogg/ogg.h"
#include "vorbis/codec.h"
#include "vorbis/vorbisfile.h"

#include "OgreOggISound.h"

namespace OgreOggSound
{
	//! A single static buffer sound (OGG) 
	/** Handles playing a sound from memory.
	*/
	class _OGGSOUND_EXPORT OgreOggStaticSound : public OgreOggISound
	{

	public:

		/** Sets the loop status.
		@remarks
			Immediately sets the loop status if a source is associated
			@param
				loop true=loop
		 */
		void loop(bool loop);
		/** Sets the source to use for playback.
		@remarks
			Sets the source object this sound will use to queue buffers onto
			for playback. Also handles refilling buffers and queuing up.
			@param
				src Source id.
		 */
		void setSource(ALuint& src);	

	protected:	

		/**
		 * Constructor
		 */
		OgreOggStaticSound(const Ogre::String& name);
		/**
		 * Destructor
		 */
		~OgreOggStaticSound();
		/** Releases buffers.
		@remarks
			Cleans up and releases OpenAL buffer objects.
		 */
		void release();	
		/** Opens audio file.
		@remarks
			Opens a specified file and checks validity. Reads first chunks
			of audio data into buffers.
			@param
				file path string
		 */
		void _openImpl(Ogre::DataStreamPtr& fileStream);
		/** Opens audio file.
		@remarks
			Uses a shared buffer.
			@param buffer
				shared buffer reference
		 */
		void _openImpl(const Ogre::String& fName, ALuint& buffer);
		/** Stops playing sound.
		@remarks
			Stops playing audio immediately. If specified to do so its source
			will be released also.
		*/
		void _stopImpl();
		/** Pauses sound.
		@remarks
			Pauses playback on the source.
		 */
		void _pauseImpl();
		/** Plays the sound.
		@remarks
			Begins playback of all buffers queued on the source. If a
			source hasn't been setup yet it is requested and initialised
			within this call.
		 */
		void _playImpl();
		/** Updates the data buffers with sound information.
		@remarks
			This function refills processed buffers with audio data from
			the stream, it automatically handles looping if set.
		 */
		void _updateAudioBuffers();
		/** Prefills buffer with audio data.
		@remarks
			Loads audio data onto the source ready for playback.
		 */
		void _prebuffer();		
		/** Calculates buffer size and format.
		@remarks
			Calculates a block aligned buffer size of 250ms using
			sound properties.
		 */
		bool _queryBufferInfo();		
		/** Releases buffers and OpenAL objects.
		@remarks
			Cleans up this sounds OpenAL objects, including buffers
			and file pointers ready for destruction.
		 */
		void _release();	

		/**
		 * Ogg file variables
		 */
		OggVorbis_File	mOggStream;			// OggVorbis file structure
		vorbis_info*	mVorbisInfo;		// Vorbis info 
		vorbis_comment* mVorbisComment;		// Vorbis comments

		Ogre::String	mAudioName;			// Name of audio file stream (Used with shared buffers)

		std::vector<char> mBufferData;		// Sound data buffer

		ALuint mBuffer;						// OpenAL buffer index
		ALenum mFormat;						// OpenAL buffer format
		ALint mPreviousOffset;				// Current play position

		friend class OgreOggSoundManager;	
	};
}