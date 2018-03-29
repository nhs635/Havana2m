
#include "QOperationTab.h"

#include <Havana2/Configuration.h>
#include <Havana2/MainWindow.h>
#include <Havana2/QStreamTab.h>
#include <Havana2/QResultTab.h>

#include <Havana2/Dialog/DeviceSetupDlg.h>

#include <DataProcess/ThreadManager.h>

#include <DataAcquisition/DataAcquisition.h>
#include <MemoryBuffer/MemoryBuffer.h>

#include <iostream>
#include <thread>


QOperationTab::QOperationTab(QWidget *parent) :
    QDialog(parent), m_pDeviceSetupDlg(nullptr)
{
	// Set main window objects
    m_pMainWnd = (MainWindow*)parent;
	m_pConfig = m_pMainWnd->m_pConfiguration;

	// Create data acquisition object
	m_pDataAcquisition = new DataAcquisition(m_pConfig);

	// Create memory buffer object
	m_pMemoryBuffer = new MemoryBuffer(this);

    // Create widgets for acquisition / recording / saving operation
    m_pToggleButton_Acquisition = new QPushButton(this);
    m_pToggleButton_Acquisition->setCheckable(true);
	m_pToggleButton_Acquisition->setMinimumSize(80, 30);
    m_pToggleButton_Acquisition->setText("Start &Acquisition");

	m_pPushButton_DeviceSetup = new QPushButton(this);
	m_pPushButton_DeviceSetup->setMinimumSize(80, 30);
	m_pPushButton_DeviceSetup->setText("&Device Setup...");

    m_pToggleButton_Recording = new QPushButton(this);
    m_pToggleButton_Recording->setCheckable(true);
	m_pToggleButton_Recording->setMinimumSize(110, 30);
    m_pToggleButton_Recording->setText("Start &Recording");
	m_pToggleButton_Recording->setDisabled(true);

    m_pToggleButton_Saving = new QPushButton(this);
    m_pToggleButton_Saving->setCheckable(true);
	m_pToggleButton_Saving->setMinimumSize(110, 30);
    m_pToggleButton_Saving->setText("&Save Recorded Data");
	m_pToggleButton_Saving->setDisabled(true);

    // Create a progress bar (general purpose?)
    m_pProgressBar = new QProgressBar(this);
    m_pProgressBar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
	m_pProgressBar->setFormat("");
    m_pProgressBar->setValue(0);

    // Set layout
    m_pVBoxLayout = new QVBoxLayout;
    m_pVBoxLayout->setSpacing(1);
	
	QHBoxLayout *pHBoxLayout1 = new QHBoxLayout;
	pHBoxLayout1->setSpacing(1);

	pHBoxLayout1->addWidget(m_pToggleButton_Acquisition);
	pHBoxLayout1->addWidget(m_pToggleButton_Recording); 

	QHBoxLayout *pHBoxLayout2 = new QHBoxLayout;
	pHBoxLayout2->setSpacing(1);

	pHBoxLayout2->addWidget(m_pPushButton_DeviceSetup);
	pHBoxLayout2->addWidget(m_pToggleButton_Saving);

    m_pVBoxLayout->addItem(pHBoxLayout1);
	m_pVBoxLayout->addItem(pHBoxLayout2);
    m_pVBoxLayout->addWidget(m_pProgressBar);
    m_pVBoxLayout->addStretch(1);

    setLayout(m_pVBoxLayout);


	// Connect signal and slot
    connect(m_pToggleButton_Acquisition, SIGNAL(toggled(bool)), this, SLOT(operateDataAcquisition(bool)));
	connect(m_pToggleButton_Recording, SIGNAL(toggled(bool)), this, SLOT(operateDataRecording(bool)));
	connect(m_pToggleButton_Saving, SIGNAL(toggled(bool)), this, SLOT(operateDataSaving(bool)));
	connect(m_pPushButton_DeviceSetup, SIGNAL(clicked(bool)), this, SLOT(createDeviceSetupDlg()));
	connect(m_pMemoryBuffer, SIGNAL(finishedBufferAllocation()), this, SLOT(setAcqRecEnable()));
	connect(m_pMemoryBuffer, SIGNAL(finishedWritingThread(bool)), this, SLOT(setSaveButtonDefault(bool)));
	connect(m_pMemoryBuffer, SIGNAL(wroteSingleFrame(int)), m_pProgressBar, SLOT(setValue(int)));
}

QOperationTab::~QOperationTab()
{
    if (m_pMemoryBuffer) delete m_pMemoryBuffer;
	if (m_pDataAcquisition) delete m_pDataAcquisition;
}


void QOperationTab::changedTab(bool change)
{	
	if (change)
		m_pToggleButton_Acquisition->setChecked(!change);

	m_pToggleButton_Acquisition->setDisabled(change);
	m_pPushButton_DeviceSetup->setDisabled(change);
}

void QOperationTab::operateDataAcquisition(bool toggled)
{
	QStreamTab* pStreamTab = m_pMainWnd->m_pStreamTab;
	if (toggled) // Start Data Acquisition
	{
		if (m_pDataAcquisition->InitializeAcquistion())
		{
			// Start Thread Process
			pStreamTab->m_pThreadVisualization->startThreading();
			pStreamTab->m_pThreadOctProcess->startThreading();

			// Start Data Acquisition
			if (m_pDataAcquisition->StartAcquisition())
			{
				m_pToggleButton_Acquisition->setText("Stop &Acquisition");								

				m_pToggleButton_Acquisition->setDisabled(true);
				m_pToggleButton_Recording->setDisabled(true);
				m_pPushButton_DeviceSetup->setDisabled(true);

				if (m_pDeviceSetupDlg != nullptr)
					m_pDeviceSetupDlg->close();

				std::thread allocate_writing_buffer([&]() {			
					m_pMemoryBuffer->allocateWritingBuffer();
				});
				allocate_writing_buffer.detach();
			}
		}
		else
			m_pToggleButton_Acquisition->setChecked(false); // When initialization is failed...
	}
	else // Stop Data Acquisition
	{
		// Stop Thread Process
		m_pDataAcquisition->StopAcquisition();
		pStreamTab->m_pThreadOctProcess->stopThreading();
		pStreamTab->m_pThreadVisualization->stopThreading();

		m_pToggleButton_Acquisition->setText("Start &Acquisition");
		m_pToggleButton_Recording->setDisabled(true);
		m_pPushButton_DeviceSetup->setEnabled(true);
	}
}

void QOperationTab::operateDataRecording(bool toggled)
{
	if (toggled) // Start Data Recording
	{
		if (m_pMemoryBuffer->startRecording())
		{
			m_pToggleButton_Recording->setText("Stop &Recording");
			m_pToggleButton_Acquisition->setDisabled(true);
			m_pToggleButton_Saving->setDisabled(true);
		}
		else
		{
			m_pToggleButton_Recording->setChecked(false);

			if (m_pMemoryBuffer->m_bIsSaved) // When select "discard"
				m_pToggleButton_Saving->setDisabled(true);
		}
	}
	else // Stop DataRecording
	{
		m_pMemoryBuffer->stopRecording();

		m_pToggleButton_Recording->setText("Start &Recording");
		m_pToggleButton_Acquisition->setEnabled(true);
		m_pToggleButton_Saving->setEnabled(m_pMemoryBuffer->m_nRecordedFrames != 0);
		m_pMainWnd->m_pResultTab->getRadioInBuffer()->setEnabled(m_pMemoryBuffer->m_nRecordedFrames != 0);

		if (m_pMemoryBuffer->m_nRecordedFrames > 1)
			m_pProgressBar->setRange(0, m_pMemoryBuffer->m_nRecordedFrames - 1);
		else
			m_pProgressBar->setRange(0, 1);
		m_pProgressBar->setValue(0);
	}
}

void QOperationTab::operateDataSaving(bool toggled)
{
	if (toggled)
	{
		if (m_pMemoryBuffer->startSaving())
		{
			m_pToggleButton_Saving->setText("Saving...");
			m_pToggleButton_Recording->setDisabled(true);
			m_pToggleButton_Saving->setDisabled(true);
			m_pProgressBar->setFormat("Writing recorded data... %p%");
		}
		else
			m_pToggleButton_Saving->setChecked(false);
	}
}

void QOperationTab::createDeviceSetupDlg()
{
	if (m_pDeviceSetupDlg == nullptr)
	{
		m_pDeviceSetupDlg = new DeviceSetupDlg(this);
		connect(m_pDeviceSetupDlg, SIGNAL(finished(int)), this, SLOT(deleteDeviceSetupDlg()));
		m_pDeviceSetupDlg->show();
	}
	m_pDeviceSetupDlg->raise();
	m_pDeviceSetupDlg->activateWindow();
}

void QOperationTab::deleteDeviceSetupDlg()
{
	m_pDeviceSetupDlg->deleteLater();
	m_pDeviceSetupDlg = nullptr;
}

void QOperationTab::setAcqRecEnable()
{
	m_pToggleButton_Acquisition->setEnabled(true); 
	m_pToggleButton_Recording->setEnabled(true);
}


void QOperationTab::setSaveButtonDefault(bool error)
{
	m_pProgressBar->setFormat("");
	m_pToggleButton_Saving->setText("&Save Recorded Data");
	m_pToggleButton_Saving->setChecked(false);
	m_pToggleButton_Saving->setDisabled(!error);
	if (m_pToggleButton_Acquisition->isChecked())
		m_pToggleButton_Recording->setEnabled(true);
}
