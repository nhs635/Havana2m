
#include "DataAcquisition.h"


DataAcquisition::DataAcquisition(Configuration* pConfig)
#if NIIMAQ_ENABLE
    : pFrameGrabber(nullptr)
#endif
{
	m_pConfig = pConfig;
#if NIIMAQ_ENABLE
	pFrameGrabber = new NI_FrameGrabber;
	pFrameGrabber->DidStopData += [&]() { pFrameGrabber->_running = false; };
#endif
}

DataAcquisition::~DataAcquisition()
{
#if NIIMAQ_ENABLE
    if (pFrameGrabber) delete pFrameGrabber;
#endif
}


bool DataAcquisition::InitializeAcquistion()
{
#if NIIMAQ_ENABLE
    // Parameter settings for frame grabber & spectrometer
	pFrameGrabber->offset = m_pConfig->offset;
	pFrameGrabber->gain = m_pConfig->gain;
	pFrameGrabber->lineTime = m_pConfig->lineTime;
	pFrameGrabber->integTime = m_pConfig->integTime;

	pFrameGrabber->acqRect(m_pConfig->acqLeft, m_pConfig->acqTop, m_pConfig->acqWidth, m_pConfig->acqHeight);

    // Initialization for frame grabber & spectrometer
    if (!(pFrameGrabber->initializeFrameGrabber()))
    {
        StopAcquisition();
        return false;
    }
	return true;    
#else
    return false;
#endif
}

bool DataAcquisition::StartAcquisition()
{
#if NIIMAQ_ENABLE
    // Start acquisition
    if (!(pFrameGrabber->startGrab()))
    {
		StopAcquisition();
        return false;
    }
    return true;
#else
    return false;
#endif
}

void DataAcquisition::StopAcquisition()
{
#if NIIMAQ_ENABLE
    // Stop thread
	pFrameGrabber->stopGrab();
#endif
}