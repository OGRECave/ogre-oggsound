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

#include "OgreOggSoundRecord.h"

using namespace Ogre;

namespace OgreOggSound
{

		OgreOggSoundRecord::OgreOggSoundRecord(ALCdevice& alDevice) :
mDevice(&alDevice)
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
void	OgreOggSoundRecord::_updateRecording()
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

const OgreOggSoundRecord::RecordDeviceList& OgreOggSoundRecord::getCaptureDeviceList() 
{
	mDeviceList.clear();
	// Get list of available Capture Devices
	const ALchar *pDeviceList = alcGetString(NULL, ALC_CAPTURE_DEVICE_SPECIFIER);
	if (pDeviceList)
	{
		while (*pDeviceList)
		{
			mDeviceList.push_back(Ogre::String(pDeviceList));
			pDeviceList += strlen(pDeviceList) + 1;
		}
	}
	return mDeviceList;
}

void	OgreOggSoundRecord::setOutputFilename(const Ogre::String& name)
{
	mOutputFile = name;
}

bool	OgreOggSoundRecord::isCaptureAvailable()
{
	if (alcIsExtensionPresent(mDevice, "ALC_EXT_CAPTURE") == AL_FALSE)
	{
		Ogre::LogManager::getSingleton().logMessage("***--- No Capture Extension detected! ---***");
		return 0;
	}
	return true;
}

bool	OgreOggSoundRecord::create(const Ogre::String& deviceName)
{
	if ( !isCaptureAvailable() ) return false;

	// Selected device
	mDeviceName = deviceName;

	// No device specified - select default
	if ( deviceName.empty() )
	{
		// Get the name of the 'default' capture device
		mDefaultCaptureDevice = alcGetString(NULL, ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER);
		mDeviceName = mDefaultCaptureDevice;
		LogManager::getSingleton().logMessage("Selected Default Capture Device: "+mDeviceName);
	}

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

void	OgreOggSoundRecord::startRecording()
{
	// Start audio capture
	alcCaptureStart(mCaptureDevice);

	// Flag recording
	mRecording = true;
}

void	OgreOggSoundRecord::stopRecording()
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

		OgreOggSoundRecord::~OgreOggSoundRecord()
{
	// Stop recording if necessary
	if ( mRecording ) stopRecording();

	// Close the Capture Device
	alcCaptureCloseDevice(mCaptureDevice);
}


}