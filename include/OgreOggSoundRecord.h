/*---------------------------------------------------------------------------*\
** This source file is part of OgreOggSound, an OpenAL wrapper library for   
** use with the Ogre Rendering Engine.										 
**                                                                           
** Copyright 2008 Ian Stangoe & Eric Boissard								 
**                                                                           
** OgreOggSound is free software: you can redistribute it and/or modify		  
** it under the terms of the GNU Lesser General Public License as published	 
** by the Free Software Foundation, either version 3 of the License, or		 
** (at your option) any later version.										 
**																			 
** OgreOggSound is distributed in the hope that it will be useful,			 
** but WITHOUT ANY WARRANTY; without even the implied warranty of			 
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the			 
** GNU Lesser General Public License for more details.						 
**																			 
** You should have received a copy of the GNU Lesser General Public License	 
** along with OgreOggSound.  If not, see <http://www.gnu.org/licenses/>.	 
\*---------------------------------------------------------------------------*/

#pragma once

#include "OgreOggSoundPreReqs.h"
#include "Ogre.h"
#include "mmreg.h"
#include <OgreString.h>

namespace OgreOggSound
{
	typedef struct
	{
		char			szRIFF[4];
		long			lRIFFSize;
		char			szWave[4];
		char			szFmt[4];
		long			lFmtSize;
		WAVEFORMATEX	wfex;
		char			szData[4];
		long			lDataSize;
	} WAVEHEADER;

	class _OGGSOUND_EXPORT OgreOggSoundRecord
	{
	
	public:

		typedef std::vector<Ogre::String> RecordDeviceList;

	private:

		ALCdevice*			mDevice;
		ALCcontext*			mContext;
		ALCdevice*			mCaptureDevice;
		const ALCchar*		mDefaultCaptureDevice;
		ALint				mSamplesAvailable;
		FILE*				mFile;
		ALchar*				mBuffer;
		WAVEHEADER			mWaveHeader;
		ALint				mDataSize;
		ALint				mSize;
		RecordDeviceList	mDeviceList;
		Ogre::String		mOutputFile;
		Ogre::String		mDeviceName;
		ALCuint				mFreq;
		ALCenum				mFormat;
		ALsizei				mBufferSize;
		unsigned short		mBitsPerSample;
		unsigned short		mNumChannels;
		bool				mRecording;

		/** Updates recording from the capture device
		*/
		void _updateRecording();
		/** Initialises a capture device ready to record audio data
		@remarks
		Gets a list of capture devices, initialises one, and opens output file
		for writing to.
		*/
		bool _openDevice();

	public:

		OgreOggSoundRecord(ALCdevice& alDevice);
		/** Gets a list of strings describing the capture devices
		*/
		const RecordDeviceList& getCaptureDeviceList();
		/** Sets the name of the file to save the captured audio to
		*/
		void setRecordingProperties(const Ogre::String& name="output.wav", ALCuint freq=44100, ALCenum format=AL_FORMAT_STEREO16, ALsizei bufferSize=4410);
		/** Creates a capture object
		*/
		bool create(const Ogre::String& devName="");
		/** Starts a recording from a capture device
		*/
		void startRecording();
		/** Returns whether a capture device is available
		*/
		bool isCaptureAvailable();
		/** Stops recording from the capture device
		@remarks
		Stops recording then writes data to a file
		*/
		void stopRecording();
		/** Gets a list of strings describing the capture devices
		*/
		~OgreOggSoundRecord();

		// Manager friend
		friend class OgreOggSoundManager;
	};

}

