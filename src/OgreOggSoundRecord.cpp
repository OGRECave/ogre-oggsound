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

#include "OgreOggSoundPreReqs.h"
#include "Ogre.h"
#include "mmreg.h"

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

using namespace Ogre;

class OgreOggSoundRecorder
{

private:

	typedef std::vector<Ogre::String> RecordDeviceList;

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
	bool				mRecording;

	/** Updates recording from the capture device
	*/
	void _updateRecording()
	{
		if ( !mRecording ) return;

		// Find out how many samples have been captured
		alcGetIntegerv(mCaptureDevice, ALC_CAPTURE_SAMPLES, 1, &mSamplesAvailable);

		// When we have enough data to fill our BUFFERSIZE byte buffer, grab the samples
		if (mSamplesAvailable > (mBufferSize / mWaveHeader.wfex.nBlockAlign))
		{
			// Consume Samples
			alcCaptureSamples(mCaptureDevice, mBuffer, mBufferSize / mWaveHeader.wfex.nBlockAlign);

			// Write the audio data to a file
			fwrite(mBuffer, mBufferSize, 1, mFile);

			// Record total amount of data recorded
			mDataSize += mBufferSize;
		}
	}


public:

	OgreOggSoundRecorder() :
	 mDevice(0)
	,mContext(0)
	,mCaptureDevice(0)
	,mDefaultCaptureDevice(0)
	,mSamplesAvailable(0)
	,mFile(0)
	,mDataSize(0)
	,mBuffer(0)
	,mSize(0)
	,mOutputFile("")
	,mFreq(22050)
	,mFormat(AL_FORMAT_MONO16)
	,mBufferSize(4410)
	,mRecording(false)
	{
	}
	/** Gets a list of strings describing the capture devices
	*/
	void _getDeviceList()
	{
		// Get list of available Capture Devices
		const ALchar *pDeviceList = alcGetString(NULL, ALC_CAPTURE_DEVICE_SPECIFIER);
		if (pDeviceList)
		{
			while (*pDeviceList)
			{
				pDeviceList += strlen(pDeviceList) + 1;
				mDeviceList.push_back(Ogre::String(pDeviceList));
			}
		}
	}

	/** Sets the name of the file to save the captured audio to
	*/
	void setOutputFilename(const Ogre::String& name)
	{
		mOutputFile = name;
	}

	/** Initialises a capture device ready to record audio data
	@remarks
		Gets a list of capture devices, initialises one, and opens output file
		for writing to.
	*/
	bool _openDevice()
	{
		if (alcIsExtensionPresent(mDevice, "ALC_EXT_CAPTURE") == AL_FALSE)
		{
			Ogre::LogManager::getSingleton().logMessage("***--- No Capture Extension detected! ---***");
			return 0;
		}

		// Get the name of the 'default' capture device
		mDefaultCaptureDevice = alcGetString(NULL, ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER);
		mDeviceName = mDefaultCaptureDevice;
		LogManager::getSingleton().logMessage("Default Capture Device is: "+mDeviceName);

		mCaptureDevice = alcCaptureOpenDevice(mDeviceName.c_str(), mFreq, mFormat, mBufferSize);
		if (mCaptureDevice)
		{
			LogManager::getSingleton().logMessage("Opened Capture Device: "+mDeviceName);

			// Create / open a file for the captured data
			mFile = fopen(mOutputFile.c_str(), "wb");

			// Prepare a WAVE file header for the captured data
			sprintf(mWaveHeader.szRIFF, "RIFF");
			mWaveHeader.lRIFFSize = 0;
			sprintf(mWaveHeader.szWave, "WAVE");
			sprintf(mWaveHeader.szFmt, "fmt ");
			mWaveHeader.lFmtSize = sizeof(WAVEFORMATEX);		
			mWaveHeader.wfex.nChannels = 1;
			mWaveHeader.wfex.wBitsPerSample = 16;
			mWaveHeader.wfex.wFormatTag = WAVE_FORMAT_PCM;
			mWaveHeader.wfex.nSamplesPerSec = mFreq;
			mWaveHeader.wfex.nBlockAlign = mWaveHeader.wfex.nChannels * mWaveHeader.wfex.wBitsPerSample / 8;
			mWaveHeader.wfex.nAvgBytesPerSec = mWaveHeader.wfex.nSamplesPerSec * mWaveHeader.wfex.nBlockAlign;
			mWaveHeader.wfex.cbSize = 0;
			sprintf(mWaveHeader.szData, "data");
			mWaveHeader.lDataSize = 0;

			fwrite(&mWaveHeader, sizeof(WAVEHEADER), 1, mFile);

			return true;
		}
		
		return false;
	}

	/** Starts a recording from a capture device
	*/
	void startRecording()
	{
		// Start audio capture
		alcCaptureStart(mCaptureDevice);

		// Flag recording
		mRecording = true;
	}

	/** Stops recording from the capture device
	@remarks
		Stops recording then writes data to a file
	*/
	void stopRecording()
	{
		if ( mRecording ) 
		{
			// Stop capture
			alcCaptureStop(mCaptureDevice);

			// Check if any Samples haven't been consumed yet
			alcGetIntegerv(mCaptureDevice, ALC_CAPTURE_SAMPLES, 1, &mSamplesAvailable);
			while (mSamplesAvailable)
			{
				if (mSamplesAvailable > (mBufferSize / mWaveHeader.wfex.nBlockAlign))
				{
					alcCaptureSamples(mCaptureDevice, mBuffer, mBufferSize / mWaveHeader.wfex.nBlockAlign);
					fwrite(mBuffer, mBufferSize, 1, mFile);
					mSamplesAvailable -= (mBufferSize / mWaveHeader.wfex.nBlockAlign);
					mDataSize += mBufferSize;
				}
				else
				{
					alcCaptureSamples(mCaptureDevice, mBuffer, mSamplesAvailable);
					fwrite(mBuffer, mSamplesAvailable * mWaveHeader.wfex.nBlockAlign, 1, mFile);
					mDataSize += mSamplesAvailable * mWaveHeader.wfex.nBlockAlign;
					mSamplesAvailable = 0;
				}
			}

			// Fill in Size information in Wave Header
			fseek(mFile, 4, SEEK_SET);
			mSize = mDataSize + sizeof(WAVEHEADER) - 8;
			fwrite(&mSize, 4, 1, mFile);
			fseek(mFile, 42, SEEK_SET);
			fwrite(&mDataSize, 4, 1, mFile);

			fclose(mFile);

			mRecording = false;
		}
	}

	/** Gets a list of strings describing the capture devices
	*/
	~OgreOggSoundRecorder()
	{
		// Stop recording if necessary
		if ( mRecording ) stopRecording();

		// Close the Capture Device
		alcCaptureCloseDevice(mCaptureDevice);
	}

};