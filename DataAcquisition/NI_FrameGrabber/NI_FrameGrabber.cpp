
#include "NI_FrameGrabber.h"

#include <windows.h> 


#if NIIMAQ_ENABLE

NI_FrameGrabber::NI_FrameGrabber() :
	interface_name("img0"), iid(0), sid(0), pid(0), bid(0),
	offset(0), gain(0), lineTime(0), integTime(0),
	acqRect(0, 0, 0, 0),
	_dirty(true), _running(false)
{
}


NI_FrameGrabber::~NI_FrameGrabber()
{
	// stop acquisition thread
	if (_thread.joinable())
	{
		_running = false;
		_thread.join();
	}

	// dispose of the sync pulse
	if (pid != 0) imgPulseDispose(pid);

	// unlock the buffers in the buffer list
	if (bid != 0) imgMemUnlock(bid);

	// dispose of the buffers 
	for (int i = 0; i < NUM_GRAB_BUFFS; i++)
		if (imaqBuffers[i] != nullptr)	imgDisposeBuffer(imaqBuffers[i]);

	// close this buffer list
	if (bid != 0) imgDisposeBufList(bid, false);
	
	// Close the interface and the session
	if (sid != 0) imgClose(sid, true);
	if (iid != 0) imgClose(iid, true);
}


bool NI_FrameGrabber::initializeFrameGrabber()
{
	if (_dirty)
	{
		if (!openFrameGrabber())
			return false;
		if (!initializeSpectrometer())
			return false;
		if (!setAcquisitionWindow())
			return false;
		if (!createAcquisitionBuffer())
			return false;
		if (!initializeSyncPulse())
			return false;

		_dirty = false;
	}

	return true;
}

bool NI_FrameGrabber::startGrab()
{
	if (_thread.joinable())
	{
		dumpError(::GetLastError(), "ERROR: Acquisition is already running: ");
		return false;
	}

	_thread = std::thread(&NI_FrameGrabber::run, this); // thread executing
	if (::SetThreadPriority(_thread.native_handle(), THREAD_PRIORITY_TIME_CRITICAL) == 0)
	{
		dumpError(::GetLastError(), "ERROR: Failed to set acquisition thread priority: ");
		return false;
	}

	printf("Frame grabbing thread is started.\n");

	return true;
}

void NI_FrameGrabber::stopGrab()
{
	if (_thread.joinable())
	{
		DidStopData();
		_thread.join();
	}

	printf("Frame grabbing thread is finished normally.\n");
}


bool NI_FrameGrabber::openFrameGrabber()
{
	// Open frame grabber
	int res;
	if ((res = imgInterfaceOpen(interface_name, &iid)) != IMAQ_SUCCESS)
	{
		dumpError(res, "Failed to open NI frame grabber: ");
		return false;
	}
	if ((res = imgSessionOpen(iid, &sid)) != IMAQ_SUCCESS)
	{
		dumpError(res, "Failed to open NI frame grabber: ");
		return false;
	}
	if ((res = imgSessionSerialFlush(sid)) != IMAQ_SUCCESS)
	{
		dumpError(res, "Failed to open NI frame grabber: ");
		return false;
	}

	printf("NI Frame Grabber is successfully opened.\n");

	return true;
}

bool NI_FrameGrabber::initializeSpectrometer()
{
	// Send messages to the spectometer to set line rate and offset
	char strOffset[_MAX_PATH] = { 0, };
	char strGain[_MAX_PATH] = { 0, };
	char strLineTime[_MAX_PATH] = { 0, };
	char strIntTime[_MAX_PATH] = { 0, };
	
	sprintf_s(strOffset, _MAX_PATH, "offset %d\r", offset);
	sprintf_s(strGain, _MAX_PATH, "gain %d\r", gain);
	sprintf_s(strLineTime, _MAX_PATH, "ltm %d\r", lineTime);
	sprintf_s(strIntTime, _MAX_PATH, "int %d\r", integTime);

	const char* strToSend[9] = { "\r", "init\r", "srst\r", strOffset, strGain,
		strIntTime, strLineTime, "ats 0\r", "lsc 1\r" };

	int res; 
	uInt32 bytesSend, bytesSent, timeOut = 1000;	
	for (int i = 0; i < 9; i++)
	{
		Sleep(100);

		char strSent[_MAX_PATH] = { 0, };
		bytesSend = GetStringLength(strToSend[i], _MAX_PATH);
		bytesSent = _MAX_PATH;

		if ((res = imgSessionSerialWrite(sid, (Int8*)strToSend[i], &bytesSend, timeOut)) != IMAQ_SUCCESS)
		{
			dumpError(res, "Failed to initialize a spectrometer: ");
			return false;
		}
		if ((res = imgSessionSerialRead(sid, strSent, &bytesSent, timeOut)) != IMAQ_SUCCESS)
		{
			dumpError(res, "Failed to initialize a spectrometer: ");
			return false;
		}

		printf("SSpectrometer initialization: %s %s \n", strToSend[i], strSent);	
	}

	printf("Wasatch Cobra Spectrometer is successfully initialized.\n");

	return true;
}

bool NI_FrameGrabber::setAcquisitionWindow()
{
	// Set Acquisition Window
	int res;
	if ((res = imgSetAttribute2(sid, IMG_ATTR_ROI_LEFT, acqRect.left)) != IMAQ_SUCCESS)
	{
		dumpError(res, "Failed to set spectrometer acquisition window: ");
		return false;
	}
	if ((res = imgSetAttribute2(sid, IMG_ATTR_ROI_TOP, acqRect.top)) != IMAQ_SUCCESS)
	{
		dumpError(res, "Failed to set spectrometer acquisition window: ");
		return false;
	}
	if ((res = imgSetAttribute2(sid, IMG_ATTR_ROI_WIDTH, acqRect.width)) != IMAQ_SUCCESS)
	{
		dumpError(res, "Failed to set spectrometer acquisition window: ");
		return false;
	}
	if ((res = imgSetAttribute2(sid, IMG_ATTR_ROWPIXELS, acqRect.width)) != IMAQ_SUCCESS)
	{
		dumpError(res, "Failed to set spectrometer acquisition window: ");
		return false;
	}
	if ((res = imgSetAttribute2(sid, IMG_ATTR_ROI_HEIGHT, acqRect.height)) != IMAQ_SUCCESS)
	{
		dumpError(res, "Failed to set spectrometer acquisition window: ");
		return false;
	}
	
	printf("Spectrometer initialization: ROI rect set up (%d, %d, %d, %d) \n", acqRect.left, acqRect.top, acqRect.width, acqRect.height);	

	return true;
}

bool NI_FrameGrabber::createAcquisitionBuffer()
{
	// Compute the size of the required buffer
	int res;
	uInt32 bytesPerPixel;
	imgGetAttribute(sid, IMG_ATTR_BYTESPERPIXEL, &bytesPerPixel);

	// Bytes per a buffer
	uInt32 buffSize = acqRect.height * acqRect.width;
	uInt32 bytesPerBuffer = buffSize * bytesPerPixel;
	
	/* Buffer Setup ********************************************************************************************************************/
	// Craete buffer list
	if ((res = imgCreateBufList(NUM_GRAB_BUFFS, &bid)) != IMAQ_SUCCESS)
	{
		dumpError(res, "Failed to create acquisition buffers: ");
		return false;
	}

	// Buffer initialization
	for (int i = 0; i < NUM_GRAB_BUFFS; i++)
	{
		// buffer for data acquisition
		if ((res = imgCreateBuffer(sid, IMG_HOST_FRAME, bytesPerBuffer, &imaqBuffers[i])) != IMAQ_SUCCESS) // Create a buffer
		{
			dumpError(res, "Failed to create acquisition buffers: ");
			return false;
		}
		if ((res = imgSetBufferElement2(bid, i, IMG_BUFF_ADDRESS, imaqBuffers[i])) != IMAQ_SUCCESS) // Specify address of a buffer
		{
			dumpError(res, "Failed to create acquisition buffers: ");
			return false;
		}
		if ((res = imgSetBufferElement2(bid, i, IMG_BUFF_SIZE, bytesPerBuffer)) != IMAQ_SUCCESS) // Specify size of a buffer
		{
			dumpError(res, "Failed to create acquisition buffers: ");
			return false;
		}

		int bufCmd = (i == (NUM_GRAB_BUFFS - 1)) ? IMG_CMD_LOOP : IMG_CMD_NEXT;
		if ((res = imgSetBufferElement2(bid, i, IMG_BUFF_COMMAND, bufCmd)) != IMAQ_SUCCESS) // Specify the end of buffers
		{
			dumpError(res, "Failed to create acquisition buffers: ");
			return false;
		}

		printf("Buffer initialization: Buffer #%d is created (size: %.2f MegaBytes) \n", i + 1, (double)bytesPerBuffer / 1024.0 / 1024.0);		
	}

	// lock down the buffers contained in the buffer list
	if ((res = imgMemLock(bid)) != IMAQ_SUCCESS)
	{
		dumpError(res, "Failed to create acquisition buffers: ");
		return false;
	}

	printf("NI Frame Grabber acquisition buffers are successfully allocated.\n");

	return true;
}

bool NI_FrameGrabber::initializeSyncPulse()
{
	int res;
	if ((res = imgPulseCreate2(PULSE_TIMEBASE_50MHZ, 2500, 2500, IMG_SIGNAL_STATUS, IMG_AQ_IN_PROGRESS, IMG_TRIG_POLAR_ACTIVEH,
		IMG_SIGNAL_EXTERNAL, 0, IMG_PULSE_POLAR_ACTIVEH, PULSE_MODE_TRAIN, &pid)) != IMAQ_SUCCESS)
	{
		dumpError(res, "Failed to initialize synchronization pulse: ");
		return false;
	}

	printf("NI Frame Grabber synchronization pulse is successfully initialized.\n");
	
	return true;
}


void NI_FrameGrabber::run()
{
	int res;

	// configure the session to use this buffer list
	if ((res = imgSessionConfigure(sid, bid)) != IMAQ_SUCCESS)
	{
		dumpError(res, "ERROR: Failed to configure session: ");
		return;
	}
	
	// Start pulse
	if ((res = imgPulseStart(pid, sid)) != IMAQ_SUCCESS)
	{
		dumpError(res, "ERROR: Failed to start generating pulse: ");
		return;
	}

	// Acquisition Start
	if ((res = imgSessionAcquire(sid, true, nullptr)) != IMAQ_SUCCESS) // Async니까 즉시 리턴될 것..
	// 그랩 시작과 동시에 엑스터널 트리거에서 싱크 펄스 발생, 그 펄스에 의해 갈바노 움직이기 시작.
	{
		dumpError(res, "ERROR: Failed to start acquisition: ");
		return;
	}

	uInt32 currBufNum = 0, frameIndex, bufStopIdx;
	uInt32 tickStart = 0, tickLastUpdate;
	uInt64 lineAcquired = 0;
		
	_running = true;
	while (_running)
	{
		// Wait at least for the first valid frame
		if ((res = imgGetAttribute(sid, IMG_ATTR_LAST_VALID_FRAME, &currBufNum)) != IMAQ_SUCCESS)
		{
			dumpError(res, "ERROR: Failed to wait the first valid frame: ");
			return;
		}
		currBufNum++;

		// Copy the last valid buffer
		np::Array<uint16_t, 2> frame(acqRect.width, acqRect.height);
		if ((res = imgSessionCopyBufferByNumber(sid, currBufNum, frame.raw_ptr(),
			IMG_OVERWRITE_GET_NEWEST, &frameIndex, nullptr)) != IMAQ_SUCCESS)
		{
			dumpError(res, "ERROR: Failed to copy the last valid buffer: ");
			return;
		}			
		DidAcquireData(frameIndex++, frame); // Callback function		

		// Acquisition Status
		if (!tickStart)
			tickStart = tickLastUpdate = GetTickCount();

		// Update counters
		lineAcquired += acqRect.height;
		if (currBufNum == NUM_GRAB_BUFFS) currBufNum = 0;

		// Periodically update progress
		uInt32 tickNow = GetTickCount();
		if (tickNow - tickLastUpdate > 5000)
		{
			double dRate;

			tickLastUpdate = tickNow;
			uInt32 dwElapsed = tickNow - tickStart;

			if (dwElapsed)
			{
				dRate = (lineAcquired / 1000.0) / (dwElapsed / 1000.0);

				uInt32 h = 0, m = 0, s = 0;
				if (dwElapsed >= 1000)
				{
					if ((s = dwElapsed / 1000) >= 60)	// Seconds
					{
						if ((m = s / 60) >= 60)			// Minutes
						{
							if (h = m / 60)				// Hours
								m %= 60;
						}
						s %= 60;
					}
				}

				printf("[Elapsed Time] %u:%02u:%02u [Line Rate] %3.2f KLine/s [Acquired Frames] %d frames \n", h, m, s, dRate, frameIndex);
			}
		}
	}
	
	// Abort Session
	imgPulseStop(pid);
	imgSessionAbort(sid, &bufStopIdx);
}


uInt32 NI_FrameGrabber::GetStringLength(const char* str, uInt32 dwMaxBuffSize)
{
	for (uInt32 i = 0; i < dwMaxBuffSize; i++)
	{
		if (str[i] == '\0')
			return i;
	}

	return 0;
}


void NI_FrameGrabber::dumpError(int res, const char* pPreamble)
{
	// Error message
	char* pErr = nullptr;	
	int my_res;
	char msg[MAX_MSG_LENGTH];
	memcpy(msg, pPreamble, strlen(pPreamble));

	my_res = imgShowError(res, msg);
	strcat(msg, pErr);

	printf("%s\n", msg);
	SendStatusMessage(msg);


	// stop acquisition thread
	if (_thread.joinable())
	{
		_running = false;
		_thread.join();
	}

	// Abort Session
	uInt32 bufStopIdx;
	imgSessionAbort(sid, &bufStopIdx);

	// dispose of the sync pulse
	if (pid != 0) imgPulseDispose(pid);

	// unlock the buffers in the buffer list
	if (bid != 0)	imgMemUnlock(bid);

	// dispose of the buffers 
	for (int i = 0; i < NUM_GRAB_BUFFS; i++)
		if (imaqBuffers[i] != nullptr)	imgDisposeBuffer(imaqBuffers[i]);

	// close this buffer list
	if (bid != 0) imgDisposeBufList(bid, false);

	// Close the interface and the session
	if (sid != 0) imgClose(sid, true);
	if (iid != 0) imgClose(iid, true);
}

#endif