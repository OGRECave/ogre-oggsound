/**
* @file OgreOggSoundRecord.h
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
* Implements methods for recording audio 
*/

#pragma once

#include "OgreOggSoundPrereqs.h"

#include <fstream>

namespace OgreOggSound
{
	//! WAVE file format structure
	struct wFormat
	{
		unsigned short 
			nChannels,
			wBitsPerSample,
			nBlockAlign,
			wFormatTag,
			cbSize;
		
		unsigned int
			nSamplesPerSec,
			nAvgBytesPerSec;
	};

	//! WAVE file header information
	struct WAVEHEADER
	{
		char			szRIFF[4];
		long			lRIFFSize;
		char			szWave[4];
		char			szFmt[4];
		long			lFmtSize;
		wFormat			wfex;
		char			szData[4];
		long			lDataSize;
	};

	//! Captures audio data
	/**
	@remarks
		This class can be used to capture audio data to an external file, WAV file ONLY.
		Use control panel --> Sound and Audio devices applet to select input type and volume.
		NOTE:- default file properties are - Frequency: 44.1Khz, Format: 16-bit stereo, Buffer Size: 8820 bytes.
	*/
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
		std::ofstream		mFile;
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
		/** Creates a capture object
		*/
		bool initCaptureDevice(const Ogre::String& devName="", const Ogre::String& fileName="output.wav", ALCuint freq=44100, ALCenum format=AL_FORMAT_STEREO16, ALsizei bufferSize=8820);
		/** Starts a recording from a capture device
		*/
		void startRecording();
		/** Returns whether a capture device is available
		*/
		bool isCaptureAvailable();
		/** Stops recording from the capture device
		*/
		void stopRecording();
		/** Closes capture device, outputs captured data to a file if available.
		*/
		~OgreOggSoundRecord();

		// Manager friend
		friend class OgreOggSoundManager;
	};

}

