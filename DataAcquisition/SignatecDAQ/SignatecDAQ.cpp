
#include "SignatecDAQ.h"

#if PX14_ENABLE

#include <px14.h>

using namespace std;

SignatecDAQ::SignatecDAQ() :
    nChannels(2), 
	nScans(2600),
	nAlines(1024),
	VoltRange1(PX14VOLTRNG_3_487_VPP),
	VoltRange2(PX14VOLTRNG_1_557_VPP),
	AcqRate(340),
	PreTrigger(0),
	TriggerDelay(0), 
	BootTimeBufIdx(0),
	UseVirtualDevice(false),
	UseInternalTrigger(false),
    _dirty(true), 
	_running(false), 
	_board(PX14_INVALID_HANDLE),
    dma_bufp(nullptr)
{
}


SignatecDAQ::~SignatecDAQ()
{
	if (_thread.joinable())
    {
		_running = false;
		_thread.join();
	}
	if (dma_bufp) BootBufCheckInPX14(_board, dma_bufp);
	if (_board != PX14_INVALID_HANDLE) { DisconnectFromDevicePX14(_board); _board = PX14_INVALID_HANDLE; }
}

	
bool SignatecDAQ::initialize()
{
	int result;
    const char* pPreamble = "Failed to initialize PX14400 device: ";

	printf("Initializing SignatecDAQ PX14400 device...\n");

	if (UseVirtualDevice)
	{
		// Connect to virtual(fake) device for test without real board	
		result = ConnectToVirtualDevicePX14(&_board, 0, 0); 
	}
	else
	{
		// Connect to and initialize the PX14400 device
		const int SerialNumber = 1; // 1 : first card
		result = ConnectToDevicePX14(&_board, SerialNumber);
	}

	if (result != SIG_SUCCESS)
	{
		dumpError(result, "Failed to initialize PX14400 device: ");
		return false;
	}

	// Set all hardware settings into a known state
	SetPowerupDefaultsPX14(_board);

	// Set dual channel acquisition
    if (nChannels == 2)
        result = SetActiveChannelsPX14(_board, PX14CHANNEL_DUAL);
    else if (nChannels == 1)
		result = SetActiveChannelsPX14(_board, PX14CHANNEL_ONE);
	if (result != SIG_SUCCESS)
	{
		dumpError(result, pPreamble);
		return false;
	}
   
	// Internal Clock: Set acquisition rate : 340 MHz
	SetAdcClockSourcePX14(_board, PX14CLKSRC_INT_VCO);
	result = SetInternalAdcClockRatePX14(_board, AcqRate);
	
	// External Clock: Set acquisition rate : 170 MHz
	// SetAdcClockSourcePX14(_board, PX14CLKSRC_EXTERNAL);
	// result = SetExternalClockRatePX14(_board, 340.0);

	if (SIG_SUCCESS != result)
	{
		//dumpError(result, pPreamble);
		//return false;
	}

	// Trigger parameters
	if (UseInternalTrigger)
	{
		SetTriggerSourcePX14(_board, PX14TRIGSRC_INT_CH1); // Internal trigger: for debugging 
		SetTriggerLevelAPX14(_board, 33024); // This value is changed according to environment
	    SetTriggerModePX14(_board, PX14TRIGMODE_POST_TRIGGER);
	}
	else
	{
		SetTriggerSourcePX14(_board, PX14TRIGSRC_EXT); 
	    SetTriggerModePX14(_board, PX14TRIGMODE_SEGMENTED);
	    SetSegmentSizePX14(_board, nChannels * nScans);
	}

   	SetTriggerDirectionExtPX14(_board, PX14TRIGDIR_POS); 

	// Enable digital I/O
	SetDigitalIoEnablePX14(_board, TRUE);
	SetDigitalIoModePX14(_board, PX14DIGIO_OUT_0V);

	// Check out a boot-time DMA buffer
	// Allocate a DMA buffer that will receive PCI acquisition data. By 
	//  allocating a DMA buffer, we can use the "fast" PX14400 library
	//  transfer routines for highest performance

	Sleep(500);
    result = BootBufCheckOutPX14(_board, BootTimeBufIdx, &dma_bufp, NULL);
	//result = AllocateDmaBufferPX14(_board, getDataBufferSize() * 8, &dma_bufp); 
	if (SIG_SUCCESS != result)
	{
		dumpError(result, pPreamble);
		return false;
	}	
	
	printf("SignatecDAQ PX14400 device is successfully initialized.\n");

	return true;
}

bool SignatecDAQ::set_init()
{
	if (_dirty)
	{
		if (!initialize())
			return false;

		_dirty = false;
	}

	return true;
}

int SignatecDAQ::getBootTimeBuffer(int idx)
{
	int result;
	unsigned int buffer_size;
	const char* pPreamble = "Failed to get PX14400 boot-time buffer: ";

	const int SerialNumber = 1; // 1 : first card
	result = ConnectToDevicePX14(&_board, SerialNumber);
	if (result != SIG_SUCCESS)
	{
		dumpError(result, pPreamble);
		return -1;
	}

	result = BootBufCfgGetPX14(_board, idx, &buffer_size);
	if (SIG_SUCCESS != result)
	{
		dumpError(result, pPreamble);
		return -1;
	}

	return buffer_size;
}

bool SignatecDAQ::setBootTimeBuffer(int idx, int buffer_size)
{
	int result;
	const char* pPreamble = "Failed to set PX14400 boot-time buffer: ";

	const int SerialNumber = 1; // 1 : first card
	result = ConnectToDevicePX14(&_board, SerialNumber);
	if (result != SIG_SUCCESS)
	{
		dumpError(result, pPreamble);
		return false;
	}
		
	result = BootBufCfgSetPX14(_board, idx, buffer_size);
	if (SIG_SUCCESS != result)
	{
		dumpError(result, pPreamble);
		return false;
	}	

	return true;
}

bool SignatecDAQ::startAcquisition()
{
	if (_thread.joinable())
	{
		dumpErrorSystem(::GetLastError(), "ERROR: Acquisition is already running: ");
		return false;
	}

	_thread = std::thread(&SignatecDAQ::run, this); // thread executing
	if (::SetThreadPriority(_thread.native_handle(), THREAD_PRIORITY_TIME_CRITICAL) == 0)
	{
		dumpErrorSystem(::GetLastError(), "ERROR: Failed to set acquisition thread priority: ");
		return false;
	}

	printf("Data acquisition thread is started.\n");

	return true;
}

void SignatecDAQ::stopAcquisition()
{
	if (_thread.joinable())
	{
		DidStopData();
		_thread.join();	
	}
	
	printf("Data acquisition thread is finished normally.\n");
}


// Acquisition Thread
void SignatecDAQ::run()
{
	int result;
	
	// Update pretrigger setting
	SetPreTriggerSamplesPX14(_board, PreTrigger);
	SetTriggerDelaySamplesPX14(_board, TriggerDelay);

	// Set input voltage range
	SetInputVoltRangeCh1PX14(_board, VoltRange1);
	SetInputVoltRangeCh2PX14(_board, VoltRange2);

	// Arm recording - Acquisition will begin when the PX14400 receives a trigger event.
	result = BeginBufferedPciAcquisitionPX14(_board);
	if (SIG_SUCCESS != result)
	{
		dumpError(result, "ERROR: Failed to arm recording: ");
		return;
	}
	
	unsigned loop_counter = 0; // uint32
	px14_sample_t *cur_chunkp = nullptr;
	px14_sample_t *prev_chunkp = nullptr;	
	ULONG dwTickStart = 0, dwTickLastUpdate;

    unsigned long long SamplesAcquired = 0;

	unsigned int frameIndex = 0;
	
	_running = true;
	while (_running)
	{		
		// check running status

		// Determine where new data transfer data will go. 
		cur_chunkp = dma_bufp + ((loop_counter % 8) * getDataBufferSize());

		// Start asynchronous DMA transfer of new data; this function starts
		//  the transfer and returns without waiting for it to finish. This
		//  gives us a chance to process the last batch of data in parallel
		//  with this transfer.
		result = GetPciAcquisitionDataFastPX14(_board, getDataBufferSize(), cur_chunkp, TRUE);
		if (SIG_SUCCESS != result)
		{
			dumpError(result, "ERROR: Failed to obtain PCI acquisition data: ");
			return;
		}

		// Process previous chunk data while we're transfering to
		// loop_counter > 1000 : to prevent FIFO overflow
		// if ((loop_counter % 4 == 0 || loop_counter % 4 == 2) && loop_counter > 1000)
		if ((loop_counter % 8 == 0 || loop_counter % 8 == 4) && loop_counter != 0)
		{
			if (loop_counter % 8 == 0)
				prev_chunkp = dma_bufp + 4 * getDataBufferSize(); // last half of buffer;
			else if (loop_counter % 8 == 4)
				prev_chunkp = dma_bufp; // first half of buffer
			
			// Callback
			np::Array<uint16_t, 2> frame(prev_chunkp, nChannels * nScans, nAlines);
			DidAcquireData(frameIndex++, frame); // Callback function			
		}
		
		// Wait for the asynchronous DMA transfer to complete so we can loop 
		//  back around to start a new one. Calling thread will sleep until
		//  the transfer completes
		while (true)
		{
			result = WaitForTransferCompletePX14(_board, 100); // 100 ms timeout (Wait until acquisition start)
			if (result == SIG_SUCCESS)
				break;
			else if (result == SIG_PX14_TIMED_OUT)
			{
				printf(".");
				SwitchToThread();				
			}
			else
			{
				dumpError(result, "ERROR: Failed to WaitForTransferCompletePX14: ");
				return;
			}

			if (!_running)
				break;
		}
			
		// Acquisition Status
		if (!dwTickStart) 
			dwTickStart = dwTickLastUpdate = GetTickCount();

		// Update counters
		SamplesAcquired += getDataBufferSize();
		loop_counter++;

		// Periodically update progress
		ULONG dwTickNow = GetTickCount();
		if (dwTickNow - dwTickLastUpdate > 5000)
		{
			double dRate;

			dwTickLastUpdate = dwTickNow;
			ULONG dwElapsed = dwTickNow - dwTickStart;

			if (dwElapsed)
			{
				dRate = (SamplesAcquired / 1000000.0) / (dwElapsed / 1000.0);

				unsigned h = 0, m = 0, s = 0;
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

				printf("[Elapsed Time] %u:%02u:%02u [DAQ Rate] %3.2f MS/s [Frame Rate] %.2f fps \n", h, m, s, dRate, (double)frameIndex / (double)(dwElapsed) * 1000.0);
			}
		}
	}

	// End the acquisition. Always do this since in ensures the board is 
	//  cleaned up properly	
	EndBufferedPciAcquisitionPX14(_board);	
	//if (dma_bufp) BootBufCheckInPX14(_board, dma_bufp);
	//if (dma_bufp) { FreeDmaBufferPX14(_board, dma_bufp); dma_bufp = nullptr; }	
	//DisconnectFromDevicePX14(_board);
		
	// Notify delegate to acquisition ended
	// DidStopAcquisition();
}

// Dump a PX14400 library error
void SignatecDAQ::dumpError(int res, const char* pPreamble)
{
	char *pErr = nullptr;
	int my_res;
	char msg[MAX_MSG_LENGTH];	
	memcpy(msg, pPreamble, strlen(pPreamble));

	my_res = GetErrorTextAPX14(res, &pErr, 0, _board);
	
	if ((SIG_SUCCESS == my_res) && pErr)
		strcat(msg, pErr);
	else
		strcat(msg, "Cannot obtain error message..");
	if (res == -6)
		strcat(msg, " (Please restart the computer.)");
	
	printf("%s\n", msg);
	SendStatusMessage(msg);

	EndBufferedPciAcquisitionPX14(_board);
	//if (dma_bufp) BootBufCheckInPX14(_board, dma_bufp);
	//if (dma_bufp) FreeDmaBufferPX14(_board, dma_bufp);
	//DisconnectFromDevicePX14(_board);	
}


void SignatecDAQ::dumpErrorSystem(int res, const char* pPreamble)
{
	char* pErr = nullptr;
	char msg[MAX_MSG_LENGTH];
	memcpy(msg, pPreamble, strlen(pPreamble));

	sprintf(pErr, "Error code (%d)", res);
	strcat(msg, pErr);

	printf("%s\n", msg);
	SendStatusMessage(msg);

	EndBufferedPciAcquisitionPX14(_board);
	//if (dma_bufp) BootBufCheckInPX14(_board, dma_bufp);
	//if (dma_bufp) FreeDmaBufferPX14(_board, dma_bufp);
	//DisconnectFromDevicePX14(_board);
}

#endif
