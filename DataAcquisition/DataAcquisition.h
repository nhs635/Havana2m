#ifndef DATAACQUISITION_H
#define DATAACQUISITION_H

#include <QObject>

#include <Havana2/Configuration.h>

#if NIIMAQ_ENABLE
#include <DataAcquisition/NI_FrameGrabber/NI_FrameGrabber.h>
#endif

#include <Common/array.h>
#include <Common/callback.h>


class DataAcquisition : public QObject
{
	Q_OBJECT

public:
    explicit DataAcquisition(Configuration*);
    virtual ~DataAcquisition();

public:
    bool InitializeAcquistion();
    bool StartAcquisition();
    void StopAcquisition();
	
#if NIIMAQ_ENABLE
public:
    void ConnectFrameGrabberAcquiredData(const std::function<void(int, const np::Array<uint16_t, 2>&)> &slot) { pFrameGrabber->DidAcquireData += slot; }
    void ConnectFrameGrabberStopData(const std::function<void(void)> &slot) { pFrameGrabber->DidStopData += slot; }
    void ConnectFrameGrabberSendStatusMessage(const std::function<void(const char*)> &slot) { pFrameGrabber->SendStatusMessage += slot; }
#endif

private:
	Configuration* m_pConfig;
#if NIIMAQ_ENABLE
	NI_FrameGrabber* pFrameGrabber;
#endif
};

#endif // DATAACQUISITION_H
