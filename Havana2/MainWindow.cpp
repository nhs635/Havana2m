
#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <Havana2/Configuration.h>

#include <Havana2/QOperationTab.h>
#include <Havana2/QDeviceControlTab.h>
#include <Havana2/QStreamTab.h>
#include <Havana2/QResultTab.h>

#include <Havana2/Dialog/DeviceSetupDlg.h>
#include <Havana2/Dialog/OctCalibDlg.h>
#include <Havana2/Dialog/SaveResultDlg.h>

#include <DataProcess/OCTProcess/OCTProcess.h>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
	
    // Initialize user interface
	QString windowTitle("Havana2 [%1] v%2");
	setWindowTitle(windowTitle.arg("microOCT").arg(VERSION));
	
	// Create configuration object
	m_pConfiguration = new Configuration;
	m_pConfiguration->getConfigFile("Havana2.ini");

	// Set timer for renew configuration 
	m_pTimer = new QTimer(this);
	m_pTimer->start(5*60*1000); // renew per 5 min

    // Create tabs objects
    m_pOperationTab = new QOperationTab(this);
	m_pDeviceControlTab = new QDeviceControlTab(this);
    m_pStreamTab = new QStreamTab(this);
	m_pResultTab = new QResultTab(this);

    // Create group boxes and tab widgets
    m_pGroupBox_OperationTab = new QGroupBox("Operation Tab");
    m_pGroupBox_OperationTab->setLayout(m_pOperationTab->getLayout());
    m_pGroupBox_OperationTab->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

    m_pGroupBox_DeviceControlTab = new QGroupBox("Device Control Tab");
    m_pGroupBox_DeviceControlTab->setLayout(m_pDeviceControlTab->getLayout());
    m_pGroupBox_DeviceControlTab->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    m_pTabWidget = new QTabWidget(this);
    m_pTabWidget->addTab(m_pStreamTab, tr("&Data Streaming"));
    m_pTabWidget->addTab(m_pResultTab, tr("&Post Processing"));
	
	// Create status bar
	QLabel *pStatusLabel_Temp1 = new QLabel(this);
	m_pStatusLabel_ImagePos = new QLabel(QString("(%1, %2)").arg(0000, 4).arg(0000, 4), this);
	QLabel *pStatusLabel_Temp3 = new QLabel(this);

	pStatusLabel_Temp1->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	m_pStatusLabel_ImagePos->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	pStatusLabel_Temp3->setFrameStyle(QFrame::Panel | QFrame::Sunken);

	// then add the widget to the status bar
	statusBar()->addPermanentWidget(pStatusLabel_Temp1, 6);
	statusBar()->addPermanentWidget(m_pStatusLabel_ImagePos, 1);
	statusBar()->addPermanentWidget(pStatusLabel_Temp3, 2);

    // Set layout
    m_pGridLayout = new QGridLayout;

    m_pGridLayout->addWidget(m_pGroupBox_OperationTab, 0, 0);
    m_pGridLayout->addWidget(m_pGroupBox_DeviceControlTab, 1, 0);
    m_pGridLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding), 2, 0);
    m_pGridLayout->addWidget(m_pTabWidget, 0, 1, 3, 1);

    ui->centralWidget->setLayout(m_pGridLayout);

	// Connect signal and slot
	connect(m_pTimer, SIGNAL(timeout()), this, SLOT(onTimer()));
	connect(m_pTabWidget, SIGNAL(currentChanged(int)), this, SLOT(changedTab(int)));
}

MainWindow::~MainWindow()
{
	if (m_pConfiguration)
	{
		m_pConfiguration->setConfigFile("Havana2.ini");
		delete m_pConfiguration;
	}
	
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *e)
{	
	if (m_pOperationTab->isAcquisitionButtonToggled())
	{
		e->ignore();
		QMessageBox::critical(this, "Exit", "Stop acquisition first!");
	}
	else
	{
		if (m_pOperationTab->getDeviceSetupDlg())
			m_pOperationTab->getDeviceSetupDlg()->close();
		if (m_pStreamTab->getOctCalibDlg())
			m_pStreamTab->getOctCalibDlg()->close();
		if (m_pResultTab->getSaveResultDlg())
			m_pResultTab->getSaveResultDlg()->close();

		e->accept();
	}
}


void MainWindow::onTimer()
{
	printf("Renewed configuration data!\n");

	m_pStreamTab->m_pOCT->saveCalibration();
	m_pConfiguration->setConfigFile("Havana2.ini");
}

void MainWindow::changedTab(int index)
{
	m_pOperationTab->changedTab(index == 1);

	if (index == 1)
	{
		m_pResultTab->setWidgetsText();

		if (m_pOperationTab->getDeviceSetupDlg())
			m_pOperationTab->getDeviceSetupDlg()->close();
		if (m_pStreamTab->getOctCalibDlg())
			m_pStreamTab->getOctCalibDlg()->close();

#ifdef GALVANO_MIRROR
		if (m_pDeviceControlTab->getEnableGalvanoMirrorControl()->isChecked())
			m_pDeviceControlTab->getEnableGalvanoMirrorControl()->setChecked(false);

		m_pDeviceControlTab->setScrollBarRange(m_pResultTab->m_pCirc ? m_pResultTab->m_pCirc->alines : m_pConfiguration->nAlines);
		m_pDeviceControlTab->setScrollBarValue(m_pDeviceControlTab->getScrollBarValue());
		m_pDeviceControlTab->setScrollBarEnabled(true);
#endif
	}
	else
	{
		m_pStreamTab->setWidgetsText();

		if (m_pResultTab->getSaveResultDlg())
			m_pResultTab->getSaveResultDlg()->close();

#ifdef GALVANO_MIRROR
		if (!m_pDeviceControlTab->getEnableGalvanoMirrorControl()->isChecked())
			m_pDeviceControlTab->setScrollBarEnabled(false);

		m_pDeviceControlTab->setScrollBarRange(m_pConfiguration->nAlines);
		m_pDeviceControlTab->setScrollBarValue(m_pDeviceControlTab->getScrollBarValue());
#endif
	}
}