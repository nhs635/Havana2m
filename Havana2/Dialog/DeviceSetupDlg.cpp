
#include "DeviceSetupDlg.h"

#include <Havana2/MainWindow.h>
#include <Havana2/QOperationTab.h>
#include <Havana2/QStreamTab.h>
#include <Havana2/QResultTab.h>

#include <DataAcquisition/DataAcquisition.h>
#include <MemoryBuffer/MemoryBuffer.h>


DeviceSetupDlg::DeviceSetupDlg(QWidget *parent) :
    QDialog(parent)
{
    // Set default size & frame
    setFixedSize(280, 150);
    setWindowFlags(Qt::Tool);
	setWindowTitle("Device Setup");

    // Set main window objects
    m_pOperationTab = (QOperationTab*)parent;
    m_pMainWnd = m_pOperationTab->getMainWnd();
	m_pConfig = m_pMainWnd->m_pConfiguration;
	m_pStreamTab = m_pMainWnd->m_pStreamTab;
	m_pResultTab = m_pMainWnd->m_pResultTab;
	m_pDataAcq = m_pOperationTab->m_pDataAcquisition;
	m_pMemBuff = m_pOperationTab->m_pMemoryBuffer;

    // Create widgets for spectrometer setup
	m_pLineEdit_offset = new QLineEdit(this);
	m_pLineEdit_offset->setFixedWidth(35);
	m_pLineEdit_offset->setText(QString::number(m_pConfig->offset));
	m_pLineEdit_offset->setAlignment(Qt::AlignCenter);
	m_pLineEdit_offset->setDisabled(m_pMemBuff->m_bIsAllocatedWritingBuffer);

	m_pLineEdit_gain = new QLineEdit(this);
	m_pLineEdit_gain->setFixedWidth(35);
	m_pLineEdit_gain->setText(QString::number(m_pConfig->gain));
	m_pLineEdit_gain->setAlignment(Qt::AlignCenter);
	m_pLineEdit_gain->setDisabled(m_pMemBuff->m_bIsAllocatedWritingBuffer);

	m_pLineEdit_lineTime = new QLineEdit(this);
	m_pLineEdit_lineTime->setFixedWidth(35);
	m_pLineEdit_lineTime->setText(QString::number(m_pConfig->lineTime));
	m_pLineEdit_lineTime->setAlignment(Qt::AlignCenter);
	m_pLineEdit_lineTime->setDisabled(m_pMemBuff->m_bIsAllocatedWritingBuffer);

	m_pLineEdit_integTime = new QLineEdit(this);
	m_pLineEdit_integTime->setFixedWidth(35);
	m_pLineEdit_integTime->setText(QString::number(m_pConfig->integTime));
	m_pLineEdit_integTime->setAlignment(Qt::AlignCenter);
	m_pLineEdit_integTime->setDisabled(m_pMemBuff->m_bIsAllocatedWritingBuffer);

	m_pLineEdit_acqLeft = new QLineEdit(this);
	m_pLineEdit_acqLeft->setFixedWidth(35);
	m_pLineEdit_acqLeft->setText(QString::number(m_pConfig->acqLeft));
	m_pLineEdit_acqLeft->setAlignment(Qt::AlignCenter);
	m_pLineEdit_acqLeft->setDisabled(m_pMemBuff->m_bIsAllocatedWritingBuffer);

	m_pLineEdit_acqTop = new QLineEdit(this);
	m_pLineEdit_acqTop->setFixedWidth(35);
	m_pLineEdit_acqTop->setText(QString::number(m_pConfig->acqTop));
	m_pLineEdit_acqTop->setAlignment(Qt::AlignCenter);
	m_pLineEdit_acqTop->setDisabled(true);

	m_pLineEdit_acqWidth = new QLineEdit(this);
	m_pLineEdit_acqWidth->setFixedWidth(35);
	m_pLineEdit_acqWidth->setText(QString::number(m_pConfig->acqWidth));
	m_pLineEdit_acqWidth->setAlignment(Qt::AlignCenter);
	m_pLineEdit_acqWidth->setDisabled(m_pMemBuff->m_bIsAllocatedWritingBuffer);

	m_pLineEdit_acqHeight = new QLineEdit(this);
	m_pLineEdit_acqHeight->setFixedWidth(35);
	m_pLineEdit_acqHeight->setText(QString::number(m_pConfig->acqHeight));
	m_pLineEdit_acqHeight->setAlignment(Qt::AlignCenter);
	m_pLineEdit_acqHeight->setDisabled(m_pMemBuff->m_bIsAllocatedWritingBuffer);
	
    
    // Set layout for spectrometer setup
	QGroupBox *pGroupBox_SpectrometerSetup = new QGroupBox("Spectrometer Setup");
	pGroupBox_SpectrometerSetup->setFixedWidth(120);
	QGridLayout *pGridLayout_SpectrometerSetup = new QGridLayout;
	pGridLayout_SpectrometerSetup->setSpacing(1);

	pGridLayout_SpectrometerSetup->addWidget(new QLabel("Offset", this), 0, 0);
	pGridLayout_SpectrometerSetup->addWidget(m_pLineEdit_offset, 0, 1);
	pGridLayout_SpectrometerSetup->addWidget(new QLabel("Gain", this), 1, 0);
	pGridLayout_SpectrometerSetup->addWidget(m_pLineEdit_gain, 1, 1);
	pGridLayout_SpectrometerSetup->addWidget(new QLabel("Line Time", this), 2, 0);
	pGridLayout_SpectrometerSetup->addWidget(m_pLineEdit_lineTime, 2, 1);
	pGridLayout_SpectrometerSetup->addWidget(new QLabel("Integ Time", this), 3, 0);
	pGridLayout_SpectrometerSetup->addWidget(m_pLineEdit_integTime, 3, 1);

	pGroupBox_SpectrometerSetup->setLayout(pGridLayout_SpectrometerSetup);

	// Set layout for acquisition window setup
	QGroupBox *pGroupBox_AcquisitionWndSetup = new QGroupBox("Acquisition Win Setup");
	pGroupBox_AcquisitionWndSetup->setFixedWidth(120);
	QGridLayout *pGridLayout_AcquisitionWndSetup = new QGridLayout;
	pGridLayout_AcquisitionWndSetup->setSpacing(1);
	
	pGridLayout_AcquisitionWndSetup->addWidget(new QLabel("Left", this), 0, 0);
	pGridLayout_AcquisitionWndSetup->addWidget(m_pLineEdit_acqLeft, 0, 1);
	pGridLayout_AcquisitionWndSetup->addWidget(new QLabel("Top", this), 1, 0);
	pGridLayout_AcquisitionWndSetup->addWidget(m_pLineEdit_acqTop, 1, 1);
	pGridLayout_AcquisitionWndSetup->addWidget(new QLabel("Width", this), 2, 0);
	pGridLayout_AcquisitionWndSetup->addWidget(m_pLineEdit_acqWidth, 2, 1);
	pGridLayout_AcquisitionWndSetup->addWidget(new QLabel("Height", this), 3, 0);
	pGridLayout_AcquisitionWndSetup->addWidget(m_pLineEdit_acqHeight, 3, 1);
		
	pGroupBox_AcquisitionWndSetup->setLayout(pGridLayout_AcquisitionWndSetup);

	// Set layout
	QHBoxLayout *pHBoxLayout = new QHBoxLayout;
	pHBoxLayout->setSpacing(3);
	pHBoxLayout->addWidget(pGroupBox_SpectrometerSetup);
	pHBoxLayout->addWidget(pGroupBox_AcquisitionWndSetup);

    setLayout(pHBoxLayout);
	

    // Connect
	connect(m_pLineEdit_offset, SIGNAL(textChanged(const QString &)), this, SLOT(changeOffset(const QString &)));
	connect(m_pLineEdit_gain, SIGNAL(textChanged(const QString &)), this, SLOT(changeGain(const QString &)));
	connect(m_pLineEdit_lineTime, SIGNAL(textChanged(const QString &)), this, SLOT(changeLineTime(const QString &)));
	connect(m_pLineEdit_integTime, SIGNAL(textChanged(const QString &)), this, SLOT(changeIntegTime(const QString &)));

	connect(m_pLineEdit_acqLeft, SIGNAL(textChanged(const QString &)), this, SLOT(changeAcqLeft(const QString &)));
	connect(m_pLineEdit_acqTop, SIGNAL(textChanged(const QString &)), this, SLOT(changeAcqTop(const QString &)));
	connect(m_pLineEdit_acqWidth, SIGNAL(textChanged(const QString &)), this, SLOT(changeAcqWidth(const QString &)));
	connect(m_pLineEdit_acqHeight, SIGNAL(textChanged(const QString &)), this, SLOT(changeAcqHeight(const QString &)));
}

DeviceSetupDlg::~DeviceSetupDlg()
{
}

void DeviceSetupDlg::keyPressEvent(QKeyEvent *e)
{
	if (e->key() != Qt::Key_Escape)
		QDialog::keyPressEvent(e);
}


void DeviceSetupDlg::changeOffset(const QString &str)
{
	m_pConfig->offset = str.toInt();
}

void DeviceSetupDlg::changeGain(const QString &str)
{
	m_pConfig->gain = str.toInt();
}

void DeviceSetupDlg::changeLineTime(const QString &str)
{
	m_pConfig->lineTime = str.toInt();
}

void DeviceSetupDlg::changeIntegTime(const QString &str)
{
	m_pConfig->integTime = str.toInt();
}


void DeviceSetupDlg::changeAcqLeft(const QString &str)
{
	// Actually not used
	m_pConfig->acqLeft = str.toInt();
}

void DeviceSetupDlg::changeAcqTop(const QString &str)
{
	// Actually not used
	m_pConfig->acqTop = str.toInt();
}

void DeviceSetupDlg::changeAcqWidth(const QString &str)
{
	m_pConfig->acqWidth = str.toInt();
	
	m_pConfig->nScans = m_pConfig->acqWidth;
	m_pConfig->nScansFFT = NEAR_2_POWER((double)m_pConfig->nScans);
	m_pConfig->n2ScansFFT = m_pConfig->nScansFFT / 2;
	m_pConfig->nFrameSize = m_pConfig->nScans * m_pConfig->nAlines;
}

void DeviceSetupDlg::changeAcqHeight(const QString &str)
{
	m_pConfig->acqHeight = str.toInt();

	m_pConfig->nAlines = str.toInt();
	if ((m_pConfig->nAlines > 200) && (m_pConfig->nAlines % 4 == 0))
	{
		m_pConfig->nAlines4 = ((m_pConfig->nAlines + 3) >> 2) << 2;
		m_pConfig->nFrameSize = m_pConfig->nScans * m_pConfig->nAlines;

		m_pStreamTab->resetObjectsForAline(m_pConfig->nAlines);
		m_pResultTab->setUserDefinedAlines(m_pConfig->nAlines);
	}
	else
		printf("nAlines should be >200 and 4's multiple.\n");
}