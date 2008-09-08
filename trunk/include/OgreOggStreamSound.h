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

#pragma once

#include "OgreOggSoundPrereqs.h"
#include <string>
#include "ogg/ogg.h"
#include "vorbis/codec.h"
#include "vorbis/vorbisfile.h"
#include <al.h>

#include "OgreOggISound.h"

/**
 * Number of buffers to use for streaming
 */
#define NUM_BUFFERS 3

namespace OgreOggSound
{
	/** 
		Subclass for a streaming sound.
	 */
	class _OGGSOUND_EXPORT OgreOggStreamSound : public OgreOggISound
	{

	public:	

		/** Opens audio file.
		@remarks
			Opens a specified file and checks validity. Reads first chunks
			of audio data into buffers.
			@param
				file path string
		 */
		void open(Ogre::DataStreamPtr& fileStream);
		/** Releases buffers.
		@remarks
			Cleans up and releases OpenAL buffer objects.
		 */
		void release();	
		/** Stops playing sound.
		@remarks
			Stops playing audio immediately. If specified to do so its source
			will be released also.
		*/
		void stop();
		/** Sets the source to use for playback.
		@remarks
			Sets the source object this sound will use to queue buffers onto
			for playback. Also handles refilling buffers and queuing up.
			@param
				src Source id.
		 */
		void setSource(ALuint& src);	
		/** Pauses sound.
		@remarks
			Pauses playback on the source.
		 */
		void pause();
		/** Plays the sound.
		@remarks
			Begins playback of all buffers queued on the source. If a
			source hasn't been setup yet it is requested and initialised
			within this call.
		 */
		void play();	
		/** Updates the data buffers with sound information.
		@remarks
			This function refills processed buffers with audio data from
			the stream, it automatically handles looping if set.
		 */
		void updateAudioBuffers();
		/** Prefills buffers with audio data.
		@remarks
			Loads audio data from the stream into the predefined data
			buffers and queues them onto the source ready for playback.
		 */
		void prebuffer();		

	protected:

		/** Constructor
		@remarks
			Creates a streamed sound object for playing audio directly from
			a file stream.
			@param
				name Unique name for sound.	
		 */
		OgreOggStreamSound(const Ogre::String& name);
		/**
		 * Destructor
		 */
		~OgreOggStreamSound();
		/** Loads data from the stream into a buffer.
		@remarks
			Reads a specified chunk of data from the file stream into a
			designated buffer object.
			@param
				buffer id to load data into.
		 */
		bool _stream(ALuint buffer);
		/** Checks for OpenAL errors.
		@remarks
			Checks for an OpenAL error after an audio operation.
		 */
		void _check();
		/** Unqueues buffers from the source.
		@remarks
			Unqueues all data buffers currently queued on the associated
			source object.
		 */
		void _dequeue();

	private:

		/** Releases buffers and OpenAL objects.
		@remarks
			Cleans up this sounds OpenAL objects, including buffers
			and file pointers ready for destruction.
		 */
		void _release();	

		/**
		 * Ogg file variables
		 */
		FILE* mOggFile;						// Ogg file pointer
		OggVorbis_File mOggStream;			// OggVorbis file structure
		vorbis_info *mVorbisInfo;			// Vorbis info
		vorbis_comment *mVorbisComment;		// Vorbis comments
		ALuint mBuffers[NUM_BUFFERS];		// Sound data buffers
		ALenum mFormat;						// OpenAL format
		bool mStreamEOF;					// EOF flag
		bool mPlay;							// Play flag

		friend class OgreOggSoundManager;
	};
}