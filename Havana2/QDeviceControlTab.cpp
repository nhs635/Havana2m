
#include "QDeviceControlTab.h"

#include <Havana2/MainWindow.h>
#include <Havana2/QOperationTab.h>
#include <Havana2/QStreamTab.h>
#include <Havana2/QResultTab.h>

#ifdef GALVANO_MIRROR
#if NIDAQ_ENABLE
#include <DeviceControl/GalvoScan/GalvoScan.h>
#endif
#endif
#ifdef PULLBACK_DEVICE
#include <DeviceControl/ZaberStage/ZaberStage.h>
#include <DeviceControl/FaulhaberMotor/FaulhaberMotor.h>
#endif


QDeviceControlTab::QDeviceControlTab(QWidget *parent) :
    QDialog(parent)
{
	// Set main window objects
	m_pMainWnd = (MainWindow*)parent;
	m_pConfig = m_pMainWnd->m_pConfiguration;
	m_pOperationTab = m_pMainWnd->m_pOperationTab;


    // Create layout
    m_pVBoxLayout = new QVBoxLayout;
    m_pVBoxLayout->setSpacing(0);

#ifdef GALVANO_MIRROR
    createGalvanoMirrorControl();
#endif
#ifdef PULLBACK_DEVICE
    createZaberStageControl();
    createFaulhaberMotorControl();
#endif
	
    // Set layout
    setLayout(m_pVBoxLayout);
}

QDeviceControlTab::~QDeviceControlTab()
{
}

void QDeviceControlTab::closeEvent(QCloseEvent* e)
{
#ifdef GALVANO_MIRROR
#if NI_ENABLE
	if (m_pCheckBox_GalvanoMirrorControl->isChecked()) m_pCheckBox_GalvanoMirrorControl->setChecked(false);
#endif
#endif
#ifdef PULLBACK_DEVICE
	if (m_pCheckBox_ZaberStageControl->isChecked()) m_pCheckBox_ZaberStageControl->setChecked(false);
	if (m_pCheckBox_FaulhaberMotorControl->isChecked()) m_pCheckBox_FaulhaberMotorControl->setChecked(false);
#endif

	e->accept();
}


#ifdef GALVANO_MIRROR
void QDeviceControlTab::createGalvanoMirrorControl()
{
    // Create widgets for galvano mirror control
    QGroupBox *pGroupBox_GalvanoMirrorControl = new QGroupBox;
    QGridLayout *pGridLayout_GalvanoMirrorControl = new QGridLayout;
    pGridLayout_GalvanoMirrorControl->setSpacing(3);

    m_pCheckBox_GalvanoMirrorControl = new QCheckBox(pGroupBox_GalvanoMirrorControl);
    m_pCheckBox_GalvanoMirrorControl->setText("Enable Galvano Mirror Control");
	
	m_pToggleButton_ScanTriggering = new QPushButton(pGroupBox_GalvanoMirrorControl);
	m_pToggleButton_ScanTriggering->setCheckable(true);
	m_pToggleButton_ScanTriggering->setText("Scan Triggering Mode On");
	m_pToggleButton_ScanTriggering->setFixedWidth(150);

    m_pLineEdit_FastPeakToPeakVoltage = new QLineEdit(pGroupBox_GalvanoMirrorControl);
    m_pLineEdit_FastPeakToPeakVoltage->setFixedWidth(30);
    m_pLineEdit_FastPeakToPeakVoltage->setText(QString::number(m_pConfig->galvoFastScanVoltage, 'f', 1));
	m_pLineEdit_FastPeakToPeakVoltage->setAlignment(Qt::AlignCenter);
    m_pLineEdit_FastPeakToPeakVoltage->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    m_pLineEdit_FastOffsetVoltage = new QLineEdit(pGroupBox_GalvanoMirrorControl);
    m_pLineEdit_FastOffsetVoltage->setFixedWidth(30);
    m_pLineEdit_FastOffsetVoltage->setText(QString::number(m_pConfig->galvoFastScanVoltageOffset, 'f', 1));
	m_pLineEdit_FastOffsetVoltage->setAlignment(Qt::AlignCenter);
    m_pLineEdit_FastOffsetVoltage->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    m_pLabel_FastScanVoltage = new QLabel("Fast Scan Voltage ", pGroupBox_GalvanoMirrorControl);
    m_pLabel_FastScanPlusMinus = new QLabel("+", pGroupBox_GalvanoMirrorControl);
    m_pLabel_FastScanPlusMinus->setBuddy(m_pLineEdit_FastPeakToPeakVoltage);
    m_pLabel_FastGalvanoVoltage = new QLabel("V", pGroupBox_GalvanoMirrorControl);
    m_pLabel_FastGalvanoVoltage->setBuddy(m_pLineEdit_FastOffsetVoltage);

	m_pLineEdit_SlowPeakToPeakVoltage = new QLineEdit(pGroupBox_GalvanoMirrorControl);
	m_pLineEdit_SlowPeakToPeakVoltage->setFixedWidth(30);
	m_pLineEdit_SlowPeakToPeakVoltage->setText(QString::number(m_pConfig->galvoSlowScanVoltage, 'f', 1));
	m_pLineEdit_SlowPeakToPeakVoltage->setAlignment(Qt::AlignCenter);
	m_pLineEdit_SlowPeakToPeakVoltage->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	if (m_pConfig->galvoSlowScanVoltage == 0)
		m_pToggleButton_ScanTriggering->setDisabled(true);

	m_pLineEdit_SlowOffsetVoltage = new QLineEdit(pGroupBox_GalvanoMirrorControl);
	m_pLineEdit_SlowOffsetVoltage->setFixedWidth(30);
	m_pLineEdit_SlowOffsetVoltage->setText(QString::number(m_pConfig->galvoSlowScanVoltageOffset, 'f', 1));
	m_pLineEdit_SlowOffsetVoltage->setAlignment(Qt::AlignCenter);
	m_pLineEdit_SlowOffsetVoltage->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	m_pLabel_SlowScanVoltage = new QLabel("Slow Scan Voltage ", pGroupBox_GalvanoMirrorControl);
	m_pLabel_SlowScanPlusMinus = new QLabel("+", pGroupBox_GalvanoMirrorControl);
	m_pLabel_SlowScanPlusMinus->setBuddy(m_pLineEdit_SlowPeakToPeakVoltage);
	m_pLabel_SlowGalvanoVoltage = new QLabel("V", pGroupBox_GalvanoMirrorControl);
	m_pLabel_SlowGalvanoVoltage->setBuddy(m_pLineEdit_SlowOffsetVoltage);

	m_pLineEdit_SlowScanIncrement = new QLineEdit(pGroupBox_GalvanoMirrorControl);
	m_pLineEdit_SlowScanIncrement->setFixedWidth(40);
	m_pLineEdit_SlowScanIncrement->setText(QString::number(m_pConfig->galvoSlowScanIncrement, 'f', 3)); 
	m_pLineEdit_SlowScanIncrement->setAlignment(Qt::AlignCenter);
	m_pLineEdit_SlowScanIncrement->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	m_pLabel_SlowScanIncrement = new QLabel("Slow Scan Increment ", pGroupBox_GalvanoMirrorControl);
	m_pLabel_SlowScanIncrement->setBuddy(m_pLineEdit_SlowScanIncrement);
	m_pLabel_SlowScanIncrementVoltage = new QLabel("V", pGroupBox_GalvanoMirrorControl);
	m_pLabel_SlowScanIncrementVoltage->setBuddy(m_pLabel_SlowScanIncrement);

    m_pScrollBar_ScanAdjustment = new QScrollBar(pGroupBox_GalvanoMirrorControl);
    m_pScrollBar_ScanAdjustment->setOrientation(Qt::Horizontal);
	m_pScrollBar_ScanAdjustment->setRange(0, m_pConfig->nAlines - 1);
	m_pScrollBar_ScanAdjustment->setSingleStep(1);
	m_pScrollBar_ScanAdjustment->setPageStep(m_pScrollBar_ScanAdjustment->maximum() / 10);
	m_pScrollBar_ScanAdjustment->setFocusPolicy(Qt::StrongFocus);
	m_pScrollBar_ScanAdjustment->setDisabled(true);
	QString str; str.sprintf("Fast Scan Adjustment  %d / %d", 0, m_pConfig->nAlines - 1);
    m_pLabel_ScanAdjustment = new QLabel(str, pGroupBox_GalvanoMirrorControl);
    m_pLabel_ScanAdjustment->setBuddy(m_pScrollBar_ScanAdjustment);
	m_pLabel_ScanAdjustment->setDisabled(true);

    pGridLayout_GalvanoMirrorControl->addWidget(m_pCheckBox_GalvanoMirrorControl, 0, 0, 1, 6);

	QHBoxLayout *pHBoxLayout_ToggleButtons = new QHBoxLayout;
	pHBoxLayout_ToggleButtons->setSpacing(3);
	pHBoxLayout_ToggleButtons->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
	pHBoxLayout_ToggleButtons->addWidget(m_pToggleButton_ScanTriggering);
	pGridLayout_GalvanoMirrorControl->addItem(pHBoxLayout_ToggleButtons, 1, 0, 1, 6);

    pGridLayout_GalvanoMirrorControl->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 2, 0);
    pGridLayout_GalvanoMirrorControl->addWidget(m_pLabel_FastScanVoltage, 2, 1);
    pGridLayout_GalvanoMirrorControl->addWidget(m_pLineEdit_FastPeakToPeakVoltage, 2, 2);
    pGridLayout_GalvanoMirrorControl->addWidget(m_pLabel_FastScanPlusMinus, 2, 3);
    pGridLayout_GalvanoMirrorControl->addWidget(m_pLineEdit_FastOffsetVoltage, 2, 4);
    pGridLayout_GalvanoMirrorControl->addWidget(m_pLabel_FastGalvanoVoltage, 2, 5);
	pGridLayout_GalvanoMirrorControl->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 3, 0);
	pGridLayout_GalvanoMirrorControl->addWidget(m_pLabel_SlowScanVoltage, 3, 1);
	pGridLayout_GalvanoMirrorControl->addWidget(m_pLineEdit_SlowPeakToPeakVoltage, 3, 2);
	pGridLayout_GalvanoMirrorControl->addWidget(m_pLabel_SlowScanPlusMinus, 3, 3);
	pGridLayout_GalvanoMirrorControl->addWidget(m_pLineEdit_SlowOffsetVoltage, 3, 4);
	pGridLayout_GalvanoMirrorControl->addWidget(m_pLabel_SlowGalvanoVoltage, 3, 5);
	pGridLayout_GalvanoMirrorControl->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 4, 0);
	pGridLayout_GalvanoMirrorControl->addWidget(m_pLabel_SlowScanIncrement, 4, 1, 1, 2);
	pGridLayout_GalvanoMirrorControl->addWidget(m_pLineEdit_SlowScanIncrement, 4, 3, 1, 2);
	pGridLayout_GalvanoMirrorControl->addWidget(m_pLabel_SlowScanIncrementVoltage, 4, 5);
	
	pGridLayout_GalvanoMirrorControl->addWidget(m_pLabel_ScanAdjustment, 5, 0, 1, 6);
    pGridLayout_GalvanoMirrorControl->addWidget(m_pScrollBar_ScanAdjustment, 6, 0, 1, 6);

    pGroupBox_GalvanoMirrorControl->setLayout(pGridLayout_GalvanoMirrorControl);
    m_pVBoxLayout->addWidget(pGroupBox_GalvanoMirrorControl);

	// Connect signal and slot
	connect(m_pCheckBox_GalvanoMirrorControl, SIGNAL(toggled(bool)), this, SLOT(enableGalvanoMirror(bool)));
	connect(m_pToggleButton_ScanTriggering, SIGNAL(toggled(bool)), this, SLOT(triggering(bool)));
	connect(m_pLineEdit_FastPeakToPeakVoltage, SIGNAL(textChanged(const QString &)), this, SLOT(changeGalvoFastScanVoltage(const QString &)));
	connect(m_pLineEdit_FastOffsetVoltage, SIGNAL(textChanged(const QString &)), this, SLOT(changeGalvoFastScanVoltageOffset(const QString &)));
	connect(m_pLineEdit_SlowPeakToPeakVoltage, SIGNAL(textChanged(const QString &)), this, SLOT(changeGalvoSlowScanVoltage(const QString &)));
	connect(m_pLineEdit_SlowOffsetVoltage, SIGNAL(textChanged(const QString &)), this, SLOT(changeGalvoSlowScanVoltageOffset(const QString &)));
	connect(m_pLineEdit_SlowScanIncrement, SIGNAL(textChanged(const QString &)), this, SLOT(changeGalvoSlowScanIncrement(const QString &)));
	connect(m_pScrollBar_ScanAdjustment, SIGNAL(valueChanged(int)), this, SLOT(scanAdjusting(int)));
}
#endif

#ifdef PULLBACK_DEVICE
void QDeviceControlTab::createZaberStageControl()
{
    // Create widgets for Zaber stage control
    QGroupBox *pGroupBox_ZaberStageControl = new QGroupBox;
    QGridLayout *pGridLayout_ZaberStageControl = new QGridLayout;
    pGridLayout_ZaberStageControl->setSpacing(3);

    m_pCheckBox_ZaberStageControl = new QCheckBox(pGroupBox_ZaberStageControl);
    m_pCheckBox_ZaberStageControl->setText("Enable Zaber Stage Control");

    m_pPushButton_SetTargetSpeed = new QPushButton(pGroupBox_ZaberStageControl);
    m_pPushButton_SetTargetSpeed->setText("Target Speed");
	m_pPushButton_SetTargetSpeed->setDisabled(true);
    m_pPushButton_MoveAbsolute = new QPushButton(pGroupBox_ZaberStageControl);
    m_pPushButton_MoveAbsolute->setText("Move Absolute");
	m_pPushButton_MoveAbsolute->setDisabled(true);
    m_pPushButton_Home = new QPushButton(pGroupBox_ZaberStageControl);
    m_pPushButton_Home->setText("Home");
	m_pPushButton_Home->setFixedWidth(60);
	m_pPushButton_Home->setDisabled(true);
    m_pPushButton_Stop = new QPushButton(pGroupBox_ZaberStageControl);
    m_pPushButton_Stop->setText("Stop");
	m_pPushButton_Stop->setFixedWidth(60);
	m_pPushButton_Stop->setDisabled(true);

    m_pLineEdit_TargetSpeed = new QLineEdit(pGroupBox_ZaberStageControl);
    m_pLineEdit_TargetSpeed->setFixedWidth(25);
    m_pLineEdit_TargetSpeed->setText(QString::number(m_pConfig->zaberPullbackSpeed));
	m_pLineEdit_TargetSpeed->setAlignment(Qt::AlignCenter);
	m_pLineEdit_TargetSpeed->setDisabled(true);
    m_pLineEdit_TravelLength = new QLineEdit(pGroupBox_ZaberStageControl);
    m_pLineEdit_TravelLength->setFixedWidth(25);
    m_pLineEdit_TravelLength->setText(QString::number(m_pConfig->zaberPullbackLength));
	m_pLineEdit_TravelLength->setAlignment(Qt::AlignCenter);
	m_pLineEdit_TravelLength->setDisabled(true);

    m_pLabel_TargetSpeed = new QLabel("mm/s", pGroupBox_ZaberStageControl);
    m_pLabel_TargetSpeed->setBuddy(m_pLineEdit_TargetSpeed);
	m_pLabel_TargetSpeed->setDisabled(true);
    m_pLabel_TravelLength = new QLabel("mm", pGroupBox_ZaberStageControl);
    m_pLabel_TravelLength->setBuddy(m_pLineEdit_TravelLength);
	m_pLabel_TravelLength->setDisabled(true);

    pGridLayout_ZaberStageControl->addWidget(m_pCheckBox_ZaberStageControl, 0, 0, 1, 5);

    pGridLayout_ZaberStageControl->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 1, 0);
    pGridLayout_ZaberStageControl->addWidget(m_pPushButton_SetTargetSpeed, 1, 1, 1, 2);
    pGridLayout_ZaberStageControl->addWidget(m_pLineEdit_TargetSpeed, 1, 3);
    pGridLayout_ZaberStageControl->addWidget(m_pLabel_TargetSpeed, 1, 4);

    pGridLayout_ZaberStageControl->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 2, 0);
    pGridLayout_ZaberStageControl->addWidget(m_pPushButton_MoveAbsolute, 2, 1, 1, 2);
    pGridLayout_ZaberStageControl->addWidget(m_pLineEdit_TravelLength, 2, 3);
    pGridLayout_ZaberStageControl->addWidget(m_pLabel_TravelLength, 2, 4);

    pGridLayout_ZaberStageControl->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 3, 0);
    pGridLayout_ZaberStageControl->addWidget(m_pPushButton_Home, 3, 1);
    pGridLayout_ZaberStageControl->addWidget(m_pPushButton_Stop, 3, 2);

    pGroupBox_ZaberStageControl->setLayout(pGridLayout_ZaberStageControl);
    m_pVBoxLayout->addWidget(pGroupBox_ZaberStageControl);

	// Connect signal and slot
	connect(m_pCheckBox_ZaberStageControl, SIGNAL(toggled(bool)), this, SLOT(enableZaberStageControl(bool)));
	connect(m_pPushButton_MoveAbsolute, SIGNAL(clicked(bool)), this, SLOT(moveAbsolute()));
	connect(m_pLineEdit_TargetSpeed, SIGNAL(textChanged(const QString &)), this, SLOT(setTargetSpeed(const QString &)));
	connect(m_pLineEdit_TravelLength, SIGNAL(textChanged(const QString &)), this, SLOT(changeZaberPullbackLength(const QString &)));
	connect(m_pPushButton_Home, SIGNAL(clicked(bool)), this, SLOT(home()));
	connect(m_pPushButton_Stop, SIGNAL(clicked(bool)), this, SLOT(stop()));
}

void QDeviceControlTab::createFaulhaberMotorControl()
{
    // Create widgets for Faulhaber motor control
    QGroupBox *pGroupBox_FaulhaberMotorControl = new QGroupBox;
    QGridLayout *pGridLayout_FaulhaberMotorControl = new QGridLayout;
    pGridLayout_FaulhaberMotorControl->setSpacing(3);

    m_pCheckBox_FaulhaberMotorControl = new QCheckBox(pGroupBox_FaulhaberMotorControl);
    m_pCheckBox_FaulhaberMotorControl->setText("Enable Faulhaber Motor Control");

    m_pToggleButton_Rotate = new QPushButton(pGroupBox_FaulhaberMotorControl);
    m_pToggleButton_Rotate->setText("Rotate");
    m_pToggleButton_Rotate->setCheckable(pGroupBox_FaulhaberMotorControl);
	m_pToggleButton_Rotate->setDisabled(true);

    m_pLineEdit_RPM = new QLineEdit(pGroupBox_FaulhaberMotorControl);
    m_pLineEdit_RPM->setFixedWidth(40);
    m_pLineEdit_RPM->setText(QString::number(m_pConfig->faulhaberRpm));
	m_pLineEdit_RPM->setAlignment(Qt::AlignCenter);
    m_pLineEdit_RPM->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	m_pLineEdit_RPM->setDisabled(true);

    m_pLabel_RPM = new QLabel("RPM", pGroupBox_FaulhaberMotorControl);
    m_pLabel_RPM->setBuddy(m_pLineEdit_RPM);
	m_pLabel_RPM->setDisabled(true);

    pGridLayout_FaulhaberMotorControl->addWidget(m_pCheckBox_FaulhaberMotorControl, 0, 0, 1, 4);
    pGridLayout_FaulhaberMotorControl->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 1, 0);
    pGridLayout_FaulhaberMotorControl->addWidget(m_pToggleButton_Rotate, 1, 1);
    pGridLayout_FaulhaberMotorControl->addWidget(m_pLineEdit_RPM, 1, 2);
    pGridLayout_FaulhaberMotorControl->addWidget(m_pLabel_RPM, 1, 3);

    pGroupBox_FaulhaberMotorControl->setLayout(pGridLayout_FaulhaberMotorControl);
    m_pVBoxLayout->addWidget(pGroupBox_FaulhaberMotorControl);
	m_pVBoxLayout->addStretch(1);

	// Connect signal and slot
	connect(m_pCheckBox_FaulhaberMotorControl, SIGNAL(toggled(bool)), this, SLOT(enableFaulhaberMotorControl(bool)));
	connect(m_pToggleButton_Rotate, SIGNAL(toggled(bool)), this, SLOT(rotate(bool)));
	connect(m_pLineEdit_RPM, SIGNAL(textChanged(const QString &)), this, SLOT(changeFaulhaberRpm(const QString &)));
}
#endif


#ifdef GALVANO_MIRROR
void QDeviceControlTab::setScrollBarValue(int pos)
{ 
	m_pScrollBar_ScanAdjustment->setValue(pos); 
}

void QDeviceControlTab::setScrollBarRange(int alines)
{ 
	QString str; str.sprintf("Fast Scan Adjustment  %d / %d", m_pScrollBar_ScanAdjustment->value(), alines - 1);
	m_pLabel_ScanAdjustment->setText(str);
	m_pScrollBar_ScanAdjustment->setRange(0, alines - 1);
}

void QDeviceControlTab::setScrollBarEnabled(bool enable)
{
	m_pLabel_ScanAdjustment->setEnabled(enable);
	m_pScrollBar_ScanAdjustment->setEnabled(enable); 
}
#endif


#ifdef GALVANO_MIRROR
void QDeviceControlTab::enableGalvanoMirror(bool toggled)
{
	if (toggled)
	{
#if NIDAQ_ENABLE
		// Set text
		m_pCheckBox_GalvanoMirrorControl->setText("Disable Galvano Mirror Control");

		// Set enabled false for Galvano mirror control widgets	
		m_pToggleButton_ScanTriggering->setEnabled(false);

		m_pLabel_FastScanVoltage->setEnabled(false);
		m_pLineEdit_FastPeakToPeakVoltage->setEnabled(false);
		m_pLabel_FastScanPlusMinus->setEnabled(false);
		m_pLineEdit_FastOffsetVoltage->setEnabled(false);
		m_pLabel_FastGalvanoVoltage->setEnabled(false);

		m_pLabel_SlowScanVoltage->setEnabled(false);
		m_pLineEdit_SlowPeakToPeakVoltage->setEnabled(false);
		m_pLabel_SlowScanPlusMinus->setEnabled(false);
		m_pLineEdit_SlowOffsetVoltage->setEnabled(false);
		m_pLabel_SlowGalvanoVoltage->setEnabled(false);

		m_pLabel_SlowScanIncrement->setEnabled(false);
		m_pLineEdit_SlowScanIncrement->setEnabled(false);
		m_pLabel_SlowScanIncrementVoltage->setEnabled(false);

		m_pScrollBar_ScanAdjustment->setEnabled(true);
		m_pLabel_ScanAdjustment->setEnabled(true);
		
		// Create Galvano mirror control objects
		m_pGalvoScan = new GalvoScan;
		m_pGalvoScan->nAlines = m_pConfig->nAlines;
		m_pGalvoScan->pp_voltage_fast = m_pLineEdit_FastPeakToPeakVoltage->text().toDouble();
		m_pGalvoScan->offset_fast = m_pLineEdit_FastOffsetVoltage->text().toDouble();
		m_pGalvoScan->pp_voltage_slow = m_pLineEdit_SlowPeakToPeakVoltage->text().toDouble();
		m_pGalvoScan->offset_slow = m_pLineEdit_SlowOffsetVoltage->text().toDouble();
		m_pGalvoScan->step_slow = m_pLineEdit_SlowScanIncrement->text().toDouble();
		if (m_pGalvoScan->step_slow < 0.00001)
		{
			m_pCheckBox_GalvanoMirrorControl->setChecked(false);
			return;
		}
		
		// Start & Stop function definition
		m_pGalvoScan->stopScan += [&]() {
			m_pCheckBox_GalvanoMirrorControl->setChecked(false);
		};
		m_pGalvoScan->startScan += [&]() {
			//printf("scan start");
			//m_pGalvoScan->start();
		};

		// Initializing
		if (!m_pGalvoScan->initialize())
		{
			m_pCheckBox_GalvanoMirrorControl->setChecked(false);
			return;
		}

		if (!m_pToggleButton_ScanTriggering->isChecked())
		{
			// Start Scanning
			m_pGalvoScan->start();
		}
	}
	else
	{
		// Delete Galvano mirror control objects
		if (m_pGalvoScan)
		{
			m_pGalvoScan->stop();
			delete m_pGalvoScan;
		}

		// Set enabled false for Galvano mirror control widgets	
		if (m_pConfig->galvoSlowScanVoltage != 0)
			m_pToggleButton_ScanTriggering->setEnabled(true);

		m_pLabel_FastScanVoltage->setEnabled(true);
		m_pLineEdit_FastPeakToPeakVoltage->setEnabled(true);
		m_pLabel_FastScanPlusMinus->setEnabled(true);
		m_pLineEdit_FastOffsetVoltage->setEnabled(true);
		m_pLabel_FastGalvanoVoltage->setEnabled(true);

		m_pLabel_SlowScanVoltage->setEnabled(true);
		m_pLineEdit_SlowPeakToPeakVoltage->setEnabled(true);
		m_pLabel_SlowScanPlusMinus->setEnabled(true);
		m_pLineEdit_SlowOffsetVoltage->setEnabled(true);
		m_pLabel_SlowGalvanoVoltage->setEnabled(true);

		m_pLabel_SlowScanIncrement->setEnabled(true);
		m_pLineEdit_SlowScanIncrement->setEnabled(true);
		m_pLabel_SlowScanIncrementVoltage->setEnabled(true);
		
		m_pScrollBar_ScanAdjustment->setEnabled(false);
		m_pScrollBar_ScanAdjustment->setValue(0);
		m_pLabel_ScanAdjustment->setEnabled(false);
		QString str; str.sprintf("Fast Scan Adjustment  %d / %d", 0, m_pConfig->nAlines - 1);
		m_pLabel_ScanAdjustment->setText(str);

		// Set text
		m_pCheckBox_GalvanoMirrorControl->setText("Enable Galvano Mirror Control");
#endif
	}
}

void QDeviceControlTab::triggering(bool checked)
{
	if (checked)
	{
		// Set text
		m_pToggleButton_ScanTriggering->setText("Scan Triggering Mode Off");		
	}
	else
	{
		// Set text
		m_pToggleButton_ScanTriggering->setText("Scan Triggering Mode On");
	}
}

void QDeviceControlTab::changeGalvoFastScanVoltage(const QString &str)
{
	m_pConfig->galvoFastScanVoltage = str.toFloat();
}

void QDeviceControlTab::changeGalvoFastScanVoltageOffset(const QString &str)
{
	m_pConfig->galvoFastScanVoltageOffset = str.toFloat();
}

void QDeviceControlTab::changeGalvoSlowScanVoltage(const QString &str)
{
	m_pConfig->galvoSlowScanVoltage = str.toFloat();

	if (m_pConfig->galvoSlowScanVoltage == 0.0)
	{
		m_pToggleButton_ScanTriggering->setChecked(false);
		m_pToggleButton_ScanTriggering->setEnabled(false);
	}
	else
		m_pToggleButton_ScanTriggering->setEnabled(true);

	int nIter = int(ceil(m_pConfig->galvoSlowScanVoltage / m_pConfig->galvoSlowScanIncrement)) + 1;
	printf("Number of fast scan: %d (Estimated scan time: %.2f sec)\n", nIter, double(nIter) / (118400 / m_pConfig->nAlines));
}

void QDeviceControlTab::changeGalvoSlowScanVoltageOffset(const QString &str)
{
	m_pConfig->galvoSlowScanVoltageOffset = str.toFloat();
}

void QDeviceControlTab::changeGalvoSlowScanIncrement(const QString &str)
{
	m_pConfig->galvoSlowScanIncrement = str.toFloat();
	int nIter = int(ceil(m_pConfig->galvoSlowScanVoltage / m_pConfig->galvoSlowScanIncrement)) + 1;
	printf("Number of fast scan: %d (Estimated scan time: %.2f sec)\n", nIter, double(nIter) / (118400 / m_pConfig->nAlines));
}

void QDeviceControlTab::scanAdjusting(int horizontalShift)
{
	m_pConfig->galvoHorizontalShift = horizontalShift;

	QString str;
	if (!m_pMainWnd->m_pTabWidget->currentIndex()) // Streaming Tab
	{
		str.sprintf("Fast Scan Adjustment  %d / %d", horizontalShift, m_pConfig->nAlines - 1);
		m_pMainWnd->m_pStreamTab->invalidate();
	}
	else // Result Tab
	{
		str.sprintf("Fast Scan Adjustment  %d / %d", horizontalShift,
			(m_pMainWnd->m_pResultTab->m_pCirc ? m_pMainWnd->m_pResultTab->m_pCirc->alines : m_pConfig->nAlines) - 1);
		m_pMainWnd->m_pResultTab->invalidate();
	}

	m_pLabel_ScanAdjustment->setText(str);
}
#endif

#ifdef PULLBACK_DEVICE
void QDeviceControlTab::enableZaberStageControl(bool toggled)
{
	if (toggled)
	{		
		// Set text
		m_pCheckBox_ZaberStageControl->setText("Disable Zaber Stage Control");

		// Create Zaber stage control objects
		m_pZaberStage = new ZaberStage;
		
		// Connect stage
		if (!(m_pZaberStage->ConnectStage()))
		{
			m_pCheckBox_ZaberStageControl->setChecked(false);
			return;
		}

		// Set target speed first
		m_pZaberStage->SetTargetSpeed(m_pLineEdit_TargetSpeed->text().toDouble());

		// Set enable true for Zaber stage control widgets
		m_pPushButton_MoveAbsolute->setEnabled(true);
		m_pPushButton_SetTargetSpeed->setEnabled(true);
		m_pPushButton_Home->setEnabled(true);
		m_pPushButton_Stop->setEnabled(true);
		m_pLineEdit_TravelLength->setEnabled(true);
		m_pLineEdit_TargetSpeed->setEnabled(true);
		m_pLabel_TravelLength->setEnabled(true);
		m_pLabel_TargetSpeed->setEnabled(true);
	}
	else
	{
		// Set enable false for Zaber stage control widgets
		m_pPushButton_MoveAbsolute->setEnabled(false);
		m_pPushButton_SetTargetSpeed->setEnabled(false);
		m_pPushButton_Home->setEnabled(false);
		m_pPushButton_Stop->setEnabled(false);
		m_pLineEdit_TravelLength->setEnabled(false);
		m_pLineEdit_TargetSpeed->setEnabled(false);
		m_pLabel_TravelLength->setEnabled(false);
		m_pLabel_TargetSpeed->setEnabled(false);

		if (m_pZaberStage)
		{
			// Stop Wait Thread
			m_pZaberStage->StopWaitThread();

			// Disconnect the Stage
			m_pZaberStage->DisconnectStage();

			// Delete Zaber stage control objects
			delete m_pZaberStage;
		}

		// Set text
		m_pCheckBox_ZaberStageControl->setText("Enable Zaber Stage Control");
	}
}

void QDeviceControlTab::moveAbsolute()
{
	m_pZaberStage->MoveAbsoulte(m_pLineEdit_TravelLength->text().toDouble());
}

void QDeviceControlTab::setTargetSpeed(const QString & str)
{
	m_pZaberStage->SetTargetSpeed(str.toDouble());
	m_pConfig->zaberPullbackSpeed = str.toInt();
}

void QDeviceControlTab::changeZaberPullbackLength(const QString &str)
{
	m_pConfig->zaberPullbackLength = str.toInt();
}

void QDeviceControlTab::home()
{
	m_pZaberStage->Home();
}

void QDeviceControlTab::stop()
{
	m_pZaberStage->Stop();
}


void QDeviceControlTab::enableFaulhaberMotorControl(bool toggled)
{
	if (toggled)
	{
		// Set text
		m_pCheckBox_FaulhaberMotorControl->setText("Disable Faulhaber Motor Control");

		// Create Faulhaber motor control objects
		m_pFaulhaberMotor = new FaulhaberMotor;

		// Connect the motor
		if (!(m_pFaulhaberMotor->ConnectDevice()))
		{
			m_pCheckBox_FaulhaberMotorControl->setChecked(false);
			return;
		}
		
		// Set enable true for Faulhaber motor control widgets
		m_pToggleButton_Rotate->setEnabled(true);
		m_pLineEdit_RPM->setEnabled(true);
		m_pLabel_RPM->setEnabled(true);
	}
	else
	{
		// Set enable false for Faulhaber motor control widgets
		m_pToggleButton_Rotate->setChecked(false);
		m_pToggleButton_Rotate->setEnabled(false);
		m_pLineEdit_RPM->setEnabled(false);
		m_pLabel_RPM->setEnabled(false);
		
		if (m_pFaulhaberMotor)
		{
			// Disconnect the motor
			m_pFaulhaberMotor->DisconnectDevice();

			// Delete Faulhaber motor control objects
			delete m_pFaulhaberMotor;
		}

		// Set text
		m_pCheckBox_FaulhaberMotorControl->setText("Enable Faulhaber Motor Control");
	}
}

void QDeviceControlTab::rotate(bool toggled)
{
	if (toggled)
	{
		m_pFaulhaberMotor->RotateMotor(m_pLineEdit_RPM->text().toInt());
		m_pLineEdit_RPM->setEnabled(false);
		m_pToggleButton_Rotate->setText("Stop");
	}
	else
	{
		m_pFaulhaberMotor->StopMotor();
		m_pLineEdit_RPM->setEnabled(true);
		m_pToggleButton_Rotate->setText("Rotate");
	}
}

void QDeviceControlTab::changeFaulhaberRpm(const QString &str)
{
	m_pConfig->faulhaberRpm = str.toInt();
}
#endif
