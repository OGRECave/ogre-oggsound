/**
* @file OgreOggStreamWavSound.h
* @author  Ian Stangoe
* @version 1.16
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
* Implements methods for creating/using a streamed wav sound
*/

#pragma once

#include "OgreOggSoundPrereqs.h"
#include <string>
#include "ogg/ogg.h"
#include "vorbis/codec.h"
#include "vorbis/vorbisfile.h"

#include "OgreOggISound.h"

namespace OgreOggSound
{
	//! A single streaming sound (WAV)
	/** Handles playing a sound from a wav stream.
	*/
	class _OGGSOUND_EXPORT OgreOggStreamWavSound : public OgreOggISound
	{

		/** Sets the position of the playback cursor in seconds
		@param seconds
			Play position in seconds 
		 */
		void setPlayPosition(Ogre::Real seconds);	
		/** Sets the source to use for playback.
		@remarks
			Sets the source object this sound will use to queue buffers onto
			for playback. Also handles refilling buffers and queuing up.
			@param
				src Source id.
		 */
		void setSource(ALuint& src);	
		/** Sets the start point of a loopable section of audio.
		@remarks
			Allows user to define any start point for a loopable sound, by default this would be 0, or the 
			entire audio data, but this function can be used to offset the start of the loop. NOTE:- the sound
			will start playback from the beginning of the audio data but upon looping, if set, it will loop
			back to the new offset position.
			@param startTime
				Position in seconds to offset the loop point.
		 */
		void setLoopOffset(Ogre::Real startTime);
		/** Returns whether sound is mono
		*/
		bool isMono();

	protected:	

		/** Constructor
		@remarks
			Creates a streamed sound object for playing audio directly from
			a file stream.
			@param
				name Unique name for sound.	
		 */
		OgreOggStreamWavSound(const Ogre::String& name);
		/**
		 * Destructor
		 */
		~OgreOggStreamWavSound();
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
		/** Stops playing sound.
		@remarks
			Stops playing audio immediately and resets playback. 
			If specified to do so its source will be released also.
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
		/** Loads data from the stream into a buffer.
		@remarks
			Reads a specified chunk of data from the file stream into a
			designated buffer object.
			@param
				buffer id to load data into.
		 */
		bool _stream(ALuint buffer);
		/** Updates the data buffers with sound information.
		@remarks
			This function refills processed buffers with audio data from
			the stream, it automatically handles looping if set.
		 */
		void _updateAudioBuffers();
		/** Unqueues buffers from the source.
		@remarks
			Unqueues all data buffers currently queued on the associated
			source object.
		 */
		void _dequeue();
		/** Prefills buffers with audio data.
		@remarks
			Loads audio data from the stream into the predefined data
			buffers and queues them onto the source ready for playback.
		 */
		void _prebuffer();		
		/** Calculates buffer size and format.
		@remarks
			Calculates a block aligned buffer size of 250ms using
			sound properties.
		 */
		bool _queryBufferInfo();		
		/** handles a request to set the playback position within the stream
		@remarks
			To ensure thread safety this function performs the request within
			the thread locked update function instead of 'immediate mode' for static sounds.
		 */
		void _updatePlayPosition();		
		/** Releases buffers and OpenAL objects.
		@remarks
			Cleans up this sounds OpenAL objects, including buffers
			and file pointers ready for destruction.
		 */
		void _release();	

		/**
		 * Ogg file variables
		 */
		ALuint mBuffers[NUM_BUFFERS];		// Sound data buffers
		ALenum mFormat;						// OpenAL format
		bool mStreamEOF;					// EOF flag
		WavFormatData mFormatData;			// WAVE format structure
		unsigned long mLoopOffsetBytes;		// Loop offset in bytes

		friend class OgreOggSoundManager;
	};
}