
#include "OctCalibDlg.h"

#include <Havana2/MainWindow.h>
#include <Havana2/QStreamTab.h>

#include <DataProcess/OCTProcess/OCTProcess.h>

#include <iostream>
#include <thread>


OctCalibDlg::OctCalibDlg(QWidget *parent) :
    QDialog(parent), m_bProceed(false), m_bBeingCalibrated(false)
{
    // Set default size & frame
    setFixedSize(340, 340);
    setWindowFlags(Qt::Tool);
	setWindowTitle("OCT Calibration");

    // Set main window objects
    m_pStreamTab = (QStreamTab*)parent;
    m_pMainWnd = m_pStreamTab->getMainWnd();
	m_pConfig = m_pMainWnd->m_pConfiguration;
    m_pOCT = m_pStreamTab->m_pOCT;
	
    // Create widgets for OCT calibration (background)
    m_pPushButton_CaptureBackground = new QPushButton(this);
    m_pPushButton_CaptureBackground->setText("Capture Background");
    m_pLabel_CaptureBackground = new QLabel(this);

	QFileInfo info("bg.bin");
	char lastUpdate[100];
	if (info.exists())
	{
		QDateTime dateTime = info.lastModified();
		sprintf(lastUpdate, "Last Update: %04d-%02d-%02d %02d:%02d:%02d",
			dateTime.date().year(), dateTime.date().month(), dateTime.date().day(),
			dateTime.time().hour(), dateTime.time().minute(), dateTime.time().second());
		m_pLabel_CaptureBackground->setText(lastUpdate);
	}
	else
		m_pLabel_CaptureBackground->setText("Last Update: X");

	// Create widgets for OCT calibration (d1)
    m_pPushButton_CaptureD1 = new QPushButton(this);
    m_pPushButton_CaptureD1->setText("Capture D1");
    m_pLabel_CaptureD1 = new QLabel(this);

	info = QFileInfo("d1.bin");
	if (info.exists())
	{
		QFile file("d1.bin");
		file.open(QIODevice::ReadOnly);

		Uint16Array2 frame(m_pConfig->nScans, m_pConfig->nAlines);
		memset(frame.raw_ptr(), 0, sizeof(uint16_t) * frame.length());
		file.read(reinterpret_cast<char*>(frame.raw_ptr()), sizeof(uint16_t) * m_pConfig->nScans * m_pConfig->nAlines);
		for (int i = 0; i < m_pConfig->nScans; i++)
			m_pOCT->getFringe(0)[i] = (float)frame(i, 0);
	
		file.close();

		QDateTime dateTime = info.lastModified();
		sprintf(lastUpdate, "Last Update: %04d-%02d-%02d %02d:%02d:%02d",
			dateTime.date().year(), dateTime.date().month(), dateTime.date().day(),
			dateTime.time().hour(), dateTime.time().minute(), dateTime.time().second());
		m_pLabel_CaptureD1->setText(lastUpdate);
	}
	else
		m_pLabel_CaptureD1->setText("Last Update: X");

	// Create widgets for OCT calibration (d2)
    m_pPushButton_CaptureD2 = new QPushButton(this);
    m_pPushButton_CaptureD2->setText("Capture D2");
    m_pLabel_CaptureD2 = new QLabel(this);
    
	info = QFileInfo("d2.bin");
	if (info.exists())
	{
		QFile file("d2.bin");
		file.open(QIODevice::ReadOnly);

		Uint16Array2 frame(m_pConfig->nScans, m_pConfig->nAlines);
		memset(frame.raw_ptr(), 0, sizeof(uint16_t) * frame.length());
		file.read(reinterpret_cast<char*>(frame.raw_ptr()), sizeof(uint16_t) * m_pConfig->nScans * m_pConfig->nAlines);
		for (int i = 0; i < m_pConfig->nScans; i++)
			m_pOCT->getFringe(0)[i] = (float)frame(i, 0);
	
		file.close();
		
		QDateTime dateTime = info.lastModified();
		sprintf(lastUpdate, "Last Update: %04d-%02d-%02d %02d:%02d:%02d",
			dateTime.date().year(), dateTime.date().month(), dateTime.date().day(),
			dateTime.time().hour(), dateTime.time().minute(), dateTime.time().second());
		m_pLabel_CaptureD2->setText(lastUpdate);
	}
	else
		m_pLabel_CaptureD2->setText("Last Update: X");

	// Create widgets for OCT calibration (calibration)
    m_pPushButton_GenerateCalibration = new QPushButton(this);
    m_pPushButton_GenerateCalibration->setText("Generate Calibration");
	m_pPushButton_GenerateCalibration->setDisabled(true);
    m_pLabel_GenerateCalibration = new QLabel(this);

	info = QFileInfo("calibration.dat");
	if (info.exists())
	{
		QDateTime dateTime = info.lastModified();
		sprintf(lastUpdate, "Last Update: %04d-%02d-%02d %02d:%02d:%02d",
			dateTime.date().year(), dateTime.date().month(), dateTime.date().day(),
			dateTime.time().hour(), dateTime.time().minute(), dateTime.time().second());
		m_pLabel_GenerateCalibration->setText(lastUpdate);
	}
	else
		m_pLabel_GenerateCalibration->setText("Last Update: X");

	if (QFileInfo("d1.bin").exists() && QFileInfo("d2.bin").exists())
		m_pPushButton_GenerateCalibration->setEnabled(true);

	// Create widgets for discom value
	m_pLabel_DiscomValue = new QLabel(this);
	m_pLabel_DiscomValue->setText("Discom Value");

	m_pLineEdit_DiscomValue = new QLineEdit(this);
	m_pLineEdit_DiscomValue->setText(QString::number(m_pConfig->octDiscomVal));
	m_pLineEdit_DiscomValue->setAlignment(Qt::AlignCenter);
	m_pLineEdit_DiscomValue->setFixedWidth(30);

	// Create widgets for OCT calibration (calibration selection)
    QLabel* pLabelVoid1 = new QLabel(this);
    pLabelVoid1->setFixedWidth(55);
    pLabelVoid1->setFixedHeight(20);

    m_pLabel_Title = new QLabel(this);
    m_pLabel_Title->setText("");

    m_pPushButton_Proceed = new QPushButton(this);
	m_pPushButton_Proceed->setFixedWidth(50);
	m_pPushButton_Proceed->setText("Proceed");

    m_pScope = new QCalibScope({0, (double)m_pConfig->n2ScansFFT}, {m_pStreamTab->getOctMinDb(), m_pStreamTab->getOctMaxDb()});

    // Set layout
    QGridLayout *pGridLayout = new QGridLayout;
    pGridLayout->setSpacing(3);

    pGridLayout->addWidget(m_pPushButton_CaptureBackground, 0, 0);
    pGridLayout->addWidget(m_pLabel_CaptureBackground, 0, 1);

    pGridLayout->addWidget(m_pPushButton_CaptureD1, 1, 0);
    pGridLayout->addWidget(m_pLabel_CaptureD1, 1, 1);

    pGridLayout->addWidget(m_pPushButton_CaptureD2, 2, 0);
    pGridLayout->addWidget(m_pLabel_CaptureD2, 2, 1);

    pGridLayout->addWidget(m_pPushButton_GenerateCalibration, 3, 0);
    pGridLayout->addWidget(m_pLabel_GenerateCalibration, 3, 1);

	QHBoxLayout *pHBoxLayout = new QHBoxLayout;
	pHBoxLayout->setSpacing(3);

	pHBoxLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
	pHBoxLayout->addWidget(m_pLabel_DiscomValue);
	pHBoxLayout->addWidget(m_pLineEdit_DiscomValue);

	pGridLayout->addItem(pHBoxLayout, 4, 0, 1, 2);

    QGridLayout *pGridLayout_Scope = new QGridLayout;
    pGridLayout_Scope->setSpacing(0);

    m_pPushButton_Proceed->setEnabled(false);

    pGridLayout_Scope->addWidget(pLabelVoid1, 0, 0);
    pGridLayout_Scope->addWidget(m_pLabel_Title, 0, 1);
    pGridLayout_Scope->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 0, 2);
    pGridLayout_Scope->addWidget(m_pPushButton_Proceed, 0, 3);
    pGridLayout_Scope->addWidget(m_pScope, 1, 0, 1, 4);

    pGridLayout->addItem(pGridLayout_Scope, 5, 0, 1, 2);

    setLayout(pGridLayout);

    // Set callback
    setOctCallback();

    // Connect
    connect(m_pPushButton_CaptureBackground, SIGNAL(clicked(bool)), this, SLOT(captureBackground(void)));
    connect(m_pPushButton_CaptureD1, SIGNAL(clicked(bool)), this, SLOT(captureD1(void)));
    connect(m_pPushButton_CaptureD2, SIGNAL(clicked(bool)), this, SLOT(captureD2(void)));
    connect(m_pPushButton_GenerateCalibration, SIGNAL(clicked(bool)), this, SLOT(generateCalibration(void)));
	connect(m_pLineEdit_DiscomValue, SIGNAL(textChanged(const QString &)), this, SLOT(setDiscomValue(const QString &)));
	connect(m_pPushButton_Proceed, SIGNAL(clicked(bool)), this, SLOT(proceed(void)));
	connect(this, SIGNAL(setGenerateCalibPushButton(bool)), this, SLOT(enableGenerateCalibPushButton(bool)));
	connect(this, SIGNAL(setProceedPushButton(bool)), this, SLOT(enableProceedPushButton(bool)));
	connect(this, SIGNAL(endCalibration(void)), this, SLOT(setWidgetsEndCalibration(void)));
}

OctCalibDlg::~OctCalibDlg()
{
	OCTProcess* pOCT = m_pOCT;
	pOCT->drawGraph.clear();
	pOCT->waitForRange.clear();
	pOCT->endCalibration.clear();
}

void OctCalibDlg::closeEvent(QCloseEvent *e)
{
	if (m_bBeingCalibrated)
		e->ignore();
	else
		finished(0);
}

void OctCalibDlg::keyPressEvent(QKeyEvent *e)
{
	if (e->key() != Qt::Key_Escape)
		QDialog::keyPressEvent(e);
}


void OctCalibDlg::setOctCallback()
{
	OCTProcess* pOCT = m_pOCT;

	pOCT->drawGraph += [&](float* data, const char* title) {

		m_pScope->resetAxis({ 0, (double)m_pConfig->n2ScansFFT }, { m_pStreamTab->getOctMinDb(), m_pStreamTab->getOctMaxDb() });
        m_pScope->drawData(data);
        m_pLabel_Title->setText(title);        
	};

	pOCT->waitForRange += [&](int& start1, int& end1) {

		m_bBeingCalibrated = true;

		emit setProceedPushButton(true);
		m_pScope->m_pRenderArea->m_bSelectionAvailable = true;

		while (!m_bProceed)
		{
			start1 = m_pScope->m_pRenderArea->m_selected1[0];
			end1 = m_pScope->m_pRenderArea->m_selected1[1];
		}

		if ((start1 == 0) && (end1 == 0))
		{
			start1 = 0;
			end1 = m_pConfig->n2ScansFFT - 1;
		}

		m_bProceed = false;
		m_pScope->m_pRenderArea->m_bSelectionAvailable = false;
	};

	pOCT->endCalibration += [&]() {

		m_bBeingCalibrated = false;

		m_pScope->resetAxis({ 0, (double)m_pConfig->n2ScansFFT }, { m_pStreamTab->getOctMinDb(), m_pStreamTab->getOctMaxDb() });
		m_pScope->update();

		emit endCalibration();

		QFileInfo info("calibration.dat");
		if (info.exists())
		{
			QDateTime dateTime = info.lastModified();
			char lastUpdate[100];
			sprintf(lastUpdate, "Last Update: %04d-%02d-%02d %02d:%02d:%02d",
				dateTime.date().year(), dateTime.date().month(), dateTime.date().day(),
				dateTime.time().hour(), dateTime.time().minute(), dateTime.time().second());
			m_pLabel_GenerateCalibration->setText(lastUpdate);
		}
		else
			m_pLabel_GenerateCalibration->setText("Last Update: X");
	};
}


void OctCalibDlg::captureBackground()
{
    disconnect(this, SIGNAL(catchFringe(uint16_t*)), 0, 0);
	connect(this, SIGNAL(catchFringe(uint16_t*)), this, SLOT(caughtBackground(uint16_t*)));
}

void OctCalibDlg::captureD1()
{    
    disconnect(this, SIGNAL(catchFringe(uint16_t*)), 0, 0);
	connect(this, SIGNAL(catchFringe(uint16_t*)), this, SLOT(caughtD1(uint16_t*)));
}

void OctCalibDlg::captureD2()
{    
    disconnect(this, SIGNAL(catchFringe(uint16_t*)), 0, 0);
	connect(this, SIGNAL(catchFringe(uint16_t*)), this, SLOT(caughtD2(uint16_t*)));
}

void OctCalibDlg::generateCalibration()
{
	m_pPushButton_CaptureBackground->setDisabled(true);
	m_pPushButton_CaptureD1->setDisabled(true);
	m_pPushButton_CaptureD2->setDisabled(true);
	m_pPushButton_GenerateCalibration->setDisabled(true);

	m_pLabel_CaptureBackground->setDisabled(true);
	m_pLabel_CaptureD1->setDisabled(true);
	m_pLabel_CaptureD2->setDisabled(true);
	m_pLabel_GenerateCalibration->setDisabled(true);

	m_pLabel_DiscomValue->setDisabled(true);
	m_pLineEdit_DiscomValue->setDisabled(true);

	m_pOCT->generateCalibration(m_pConfig->octDiscomVal);
}

void OctCalibDlg::proceed()
{
	m_bProceed = true;
	memset(m_pScope->m_pRenderArea->m_selected, 0, sizeof(int) * 2);
}


void OctCalibDlg::caughtBackground(uint16_t* fringe)
{
	std::thread set_bg([&, fringe]()
	{
		// Set background
		np::Uint16Array2 frame(fringe, m_pConfig->nScans, m_pConfig->nAlines);
		m_pOCT->setBg(frame);

		// DisconnectingP
		disconnect(this, SIGNAL(catchFringe(uint16_t*)), 0, 0);

		// Plot background fringe
		np::FloatArray data(m_pConfig->nScans);
		ippsConvert_16u32f(frame.raw_ptr(), data, m_pConfig->nScans);

		m_pScope->resetAxis({ 0, (double)m_pConfig->nScans }, { 0, POWER_2(12) });
		m_pScope->drawData(data.raw_ptr());
		m_pLabel_Title->setVisible(true);
		m_pLabel_Title->setText("Background Fringe");

		// Wrtie background data
		QFile file("bg.bin");
		if (file.open(QIODevice::WriteOnly))
		{
			qint64 sizeWrote = file.write(reinterpret_cast<char*>(frame.raw_ptr()), sizeof(uint16_t) * frame.length());
			file.close();

			if (sizeWrote)
				printf("Captured background fringe.\n");
			else
				printf("Error is occurred while capturing background fringe.\n");
		}

		// Update file information
		QFileInfo info("bg.bin");
		QDateTime dateTime = info.lastModified();
		char lastUpdate[100];

		sprintf(lastUpdate, "Last Update: %04d-%02d-%02d %02d:%02d:%02d",
			dateTime.date().year(), dateTime.date().month(), dateTime.date().day(),
			dateTime.time().hour(), dateTime.time().minute(), dateTime.time().second());
		m_pLabel_CaptureBackground->setText(lastUpdate);

	});
	set_bg.detach();
}

void OctCalibDlg::caughtD1(uint16_t* fringe)
{
	std::thread set_d1([&, fringe]()
	{
		// Set d1
		np::Uint16Array2 frame(fringe, m_pConfig->nScans, m_pConfig->nAlines);
		m_pOCT->setFringe(frame, 0);

		// Disconnecting
		disconnect(this, SIGNAL(catchFringe(uint16_t*)), 0, 0);

		// Plot d1 fringe
		np::FloatArray data(m_pConfig->nScans);
		ippsConvert_16u32f(frame.raw_ptr(), data, m_pConfig->nScans);

		m_pScope->resetAxis({ 0, (double)m_pConfig->nScans }, { 0, POWER_2(12) });
		m_pScope->drawData(data.raw_ptr());
		m_pLabel_Title->setVisible(true);
		m_pLabel_Title->setText("D1 Fringe");

		// Wrtie d1 data
		QFile file("d1.bin");
		if (file.open(QIODevice::WriteOnly))
		{
			qint64 sizeWrote = file.write(reinterpret_cast<char*>(frame.raw_ptr()), sizeof(uint16_t) * frame.length());
			file.close();

			if (sizeWrote)
				printf("Captured d1 fringe.\n");
			else
				printf("Error is occurred while capturing d1 fringe.\n");
		}

		// Update file information
		QFileInfo info("d1.bin");
		QDateTime dateTime = info.lastModified();
		char lastUpdate[100];

		sprintf(lastUpdate, "Last Update: %04d-%02d-%02d %02d:%02d:%02d",
			dateTime.date().year(), dateTime.date().month(), dateTime.date().day(),
			dateTime.time().hour(), dateTime.time().minute(), dateTime.time().second());
		m_pLabel_CaptureD1->setText(lastUpdate);

		if (QFileInfo("d1.bin").exists() && QFileInfo("d2.bin").exists())
			m_pPushButton_GenerateCalibration->setEnabled(true);
	});
	set_d1.detach();
}

void OctCalibDlg::caughtD2(uint16_t* fringe)
{
	std::thread set_d2([&, fringe]()
	{
		// Set d2
		np::Uint16Array2 frame(fringe, m_pConfig->nScans, m_pConfig->nAlines);
		m_pOCT->setFringe(frame, 1);

		// Disconnecting
		disconnect(this, SIGNAL(catchFringe(uint16_t*)), 0, 0);

		// Plot d2 fringe
		np::FloatArray data(m_pConfig->nScans);
		ippsConvert_16u32f(frame.raw_ptr(), data, m_pConfig->nScans);

		m_pScope->resetAxis({ 0, (double)m_pConfig->nScans }, { 0, POWER_2(12) });
		m_pScope->drawData(data.raw_ptr());
		m_pLabel_Title->setVisible(true);
		m_pLabel_Title->setText("D2 Fringe");

		// Wrtie d2 data
		QFile file("d2.bin");
		if (file.open(QIODevice::WriteOnly))
		{
			qint64 sizeWrote = file.write(reinterpret_cast<char*>(frame.raw_ptr()), sizeof(uint16_t) * frame.length());
			file.close();

			if (sizeWrote)
				printf("Captured d2 fringe.\n");
			else
				printf("Error is occurred while capturing d2 fringe.\n");
		}

		// Update file information
		QFileInfo info("d2.bin");
		QDateTime dateTime = info.lastModified();
		char lastUpdate[100];

		sprintf(lastUpdate, "Last Update: %04d-%02d-%02d %02d:%02d:%02d",
			dateTime.date().year(), dateTime.date().month(), dateTime.date().day(),
			dateTime.time().hour(), dateTime.time().minute(), dateTime.time().second());
		m_pLabel_CaptureD2->setText(lastUpdate);

		if (QFileInfo("d1.bin").exists() && QFileInfo("d2.bin").exists())
			m_pPushButton_GenerateCalibration->setEnabled(true);
	});
	set_d2.detach();
}


void OctCalibDlg::setDiscomValue(const QString &str)
{
	m_pConfig->octDiscomVal = str.toInt();
	m_pOCT->changeDiscomValue(m_pConfig->octDiscomVal);
}

void OctCalibDlg::enableGenerateCalibPushButton(bool set)
{
	m_pPushButton_GenerateCalibration->setEnabled(set);
}

void OctCalibDlg::enableProceedPushButton(bool set)
{
	m_pPushButton_Proceed->setEnabled(set);
}

void OctCalibDlg::setWidgetsEndCalibration()
{
	m_pLabel_Title->setText("");
	m_pPushButton_Proceed->setEnabled(false);

	m_pPushButton_CaptureBackground->setEnabled(true);
	m_pPushButton_CaptureD1->setEnabled(true);
	m_pPushButton_CaptureD2->setEnabled(true);
	m_pPushButton_GenerateCalibration->setEnabled(true);

	m_pLabel_CaptureBackground->setEnabled(true);
	m_pLabel_CaptureD1->setEnabled(true);
	m_pLabel_CaptureD2->setEnabled(true);
	m_pLabel_GenerateCalibration->setEnabled(true);

	m_pLabel_DiscomValue->setEnabled(true);
	m_pLineEdit_DiscomValue->setEnabled(true);
}