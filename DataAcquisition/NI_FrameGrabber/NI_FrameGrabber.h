#ifndef _NI_FRAME_GRABBER_H_
#define _NI_FRAME_GRABBER_H_

#include <Havana2/Configuration.h>

#include <Common/array.h>
#include <Common/callback.h>

#include <iostream>
#include <thread>

#include <niimaq.h>

#define IMAQ_SUCCESS   0
#define MAX_MSG_LENGTH 2000


struct AcqRect
{
	AcqRect(uInt32 _left, uInt32 _top, uInt32 _width, uInt32 _height) :
		left(_left), top(_top), width(_width), height(_height) {}

	void operator()(uInt32 _left, uInt32 _top, uInt32 _width, uInt32 _height)
	{
		left = _left; top = _top;
		width = _width; height = _height;
	}

	uInt32 left;
	uInt32 top;
	uInt32 width;
	uInt32 height;
};

class NI_FrameGrabber
{
public:
#if NIIMAQ_ENABLE
	explicit NI_FrameGrabber();
	virtual ~NI_FrameGrabber();

	// callbacks
	callback2<int, const np::Array<uint16_t, 2> &> DidAcquireData;
	callback<void> DidStopData;
	callback<const char*> SendStatusMessage;

public:
	bool initializeFrameGrabber();
	bool startGrab();
	void stopGrab();

private:
	bool openFrameGrabber();
	bool initializeSpectrometer();
	bool setAcquisitionWindow();
	bool createAcquisitionBuffer();
	bool initializeSyncPulse();

public:
	uInt8 offset, gain, lineTime, integTime;
	AcqRect acqRect;

	bool _running;

private:
	bool _dirty;

	// thread
	std::thread _thread;
	void run();

private:
	// NI IMAQ IDs
	const char* interface_name;
	INTERFACE_ID iid;
	SESSION_ID sid;
	PULSE_ID pid;
	BUFLIST_ID bid;
	
	// Data buffers
	void* imaqBuffers[NUM_GRAB_BUFFS];
	
	// Dump a NI IMAQ error
	uInt32 GetStringLength(const char* str, uInt32 dwMaxBuffSize);
	void dumpError(int res, const char* pPreamble);
#endif
};

#endif