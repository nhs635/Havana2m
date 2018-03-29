
#include "QStreamTab.h"

#include <Havana2/MainWindow.h>
#include <Havana2/QOperationTab.h>
#include <Havana2/QDeviceControlTab.h>

#include <DataAcquisition/DataAcquisition.h>
#include <MemoryBuffer/MemoryBuffer.h>

#include <Havana2/Viewer/QScope.h>
#include <Havana2/Viewer/QImageView.h>

#include <DataProcess/OCTProcess/OCTProcess.h>
#include <DataProcess/ThreadManager.h>

#include <Havana2/Dialog/OctCalibDlg.h>


QStreamTab::QStreamTab(QWidget *parent) :
    QDialog(parent), m_pOctCalibDlg(nullptr), m_pImgObjRectImage(nullptr), m_pImgObjCircImage(nullptr),
	m_pCirc(nullptr), m_pMedfilt(nullptr)
{
	// Set main window objects
	m_pMainWnd = (MainWindow*)parent;
	m_pConfig = m_pMainWnd->m_pConfiguration;
	m_pOperationTab = m_pMainWnd->m_pOperationTab;
	m_pDataAcq = m_pOperationTab->m_pDataAcquisition;
	m_pMemBuff = m_pOperationTab->m_pMemoryBuffer;

	// Create data process object
	m_pOCT = new OCTProcess(m_pConfig->nScans, m_pConfig->nAlines);
	m_pOCT->loadCalibration();

	// Create thread managers for data processing
	m_pThreadOctProcess = new ThreadManager("OCT image process");
	m_pThreadVisualization = new ThreadManager("Visualization process");

	// Create buffers for threading operation
    m_pMemBuff->m_syncBuffering.allocate_queue_buffer(m_pConfig->nScans, m_pConfig->nAlines, PROCESSING_BUFFER_SIZE);
    m_syncOctProcessing.allocate_queue_buffer(m_pConfig->nScans, m_pConfig->nAlines, PROCESSING_BUFFER_SIZE); // OCT Processing
    m_syncVisualization.allocate_queue_buffer(m_pConfig->n2ScansFFT, m_pConfig->nAlines, PROCESSING_BUFFER_SIZE); // Visualization
	
	// Set signal object
	setDataAcquisitionCallback();
	setOctProcessingCallback();
	setVisualizationCallback();

	// Create visualization buffers
	m_visFringe = np::FloatArray2(m_pConfig->nScans, m_pConfig->nAlines);
	m_visImage = np::FloatArray2(m_pConfig->n2ScansFFT, m_pConfig->nAlines);

	// Create image visualization buffers
	ColorTable temp_ctable;
	m_pImgObjRectImage = new ImageObject(m_pConfig->nAlines, m_pConfig->n2ScansFFT, temp_ctable.m_colorTableVector.at(temp_ctable.gray));
	m_pImgObjCircImage = new ImageObject(2 * CIRC_RADIUS, 2 * CIRC_RADIUS, temp_ctable.m_colorTableVector.at(temp_ctable.gray));
	
	m_pCirc = new circularize(CIRC_RADIUS, m_pConfig->nAlines, false);
	m_pMedfilt = new medfilt(m_pConfig->nAlines, m_pConfig->n2ScansFFT, 3, 3);


    // Create layout
    QHBoxLayout* pHBoxLayout = new QHBoxLayout;
	pHBoxLayout->setSpacing(0);

    // Create graph view
	m_pScope_OctFringe = new QScope({ 0, (double)m_pConfig->nScans }, { 0, POWER_2(12) }, 2, 2, 1, 1, 0, 0, "", "");
	m_pScope_OctFringe->setMinimumSize(600, 250);
	m_pScope_OctDepthProfile = new QScope({ 0, (double)m_pConfig->n2ScansFFT }, { (double)m_pConfig->octDbRange.min, (double)m_pConfig->octDbRange.max }, 2, 2, 1, 1, 0, 0, "", "dB");
	m_pScope_OctDepthProfile->setMinimumSize(600, 250);
	
    // Create slider for exploring a-lines
    m_pSlider_SelectAline = new QSlider(this);
    m_pSlider_SelectAline->setOrientation(Qt::Horizontal);
    m_pSlider_SelectAline->setRange(0, m_pConfig->nAlines - 1);

    m_pLabel_SelectAline = new QLabel(this);
    QString str; str.sprintf("Current A-line : %4d / %4d   ", 1, m_pConfig->nAlines);
    m_pLabel_SelectAline->setText(str);
    m_pLabel_SelectAline->setBuddy(m_pSlider_SelectAline);

    // Set layout for left panel
    QVBoxLayout *pVBoxLayout_GraphView = new QVBoxLayout;
    pVBoxLayout_GraphView->setSpacing(0);

	QHBoxLayout *pHBoxLayout_SelectAline = new QHBoxLayout;
	pHBoxLayout_SelectAline->addWidget(m_pLabel_SelectAline);
	pHBoxLayout_SelectAline->addWidget(m_pSlider_SelectAline);

	pVBoxLayout_GraphView->addWidget(m_pScope_OctFringe);
	pVBoxLayout_GraphView->addItem(pHBoxLayout_SelectAline);
	pVBoxLayout_GraphView->addWidget(m_pScope_OctDepthProfile);

    pHBoxLayout->addItem(pVBoxLayout_GraphView);

    // Create OCT visualization option tab
    createOctVisualizationOptionTab();
	
    // Create image view
	m_pImageView_RectImage = new QImageView(ColorTable::colortable(m_pConfig->octColorTable), m_pConfig->nAlines, m_pConfig->n2ScansFFT);
	m_pImageView_CircImage = new QImageView(ColorTable::colortable(m_pConfig->octColorTable), 2 * CIRC_RADIUS, 2 * CIRC_RADIUS);

    m_pImageView_RectImage->setMinimumSize(350, 350);
    m_pImageView_RectImage->setMovedMouseCallback([&] (QPoint& p) { m_pMainWnd->m_pStatusLabel_ImagePos->setText(QString("(%1, %2)").arg(p.x(), 4).arg(p.y(), 4)); });

    m_pImageView_CircImage->setMinimumSize(350, 350);
    m_pImageView_CircImage->setSquare(true);
	m_pImageView_CircImage->hide();
    //m_pImageView_CircImage->setVisible(false);
    m_pImageView_CircImage->setMovedMouseCallback([&] (QPoint& p) { m_pMainWnd->m_pStatusLabel_ImagePos->setText(QString("(%1, %2)").arg(p.x(), 4).arg(p.y(), 4)); });

    // Set layout for right panel
	QVBoxLayout *pVBoxLayout_RightPanel = new QVBoxLayout;
	pVBoxLayout_RightPanel->setSpacing(0);

	pVBoxLayout_RightPanel->addWidget(m_pGroupBox_OctVisualization);
	pVBoxLayout_RightPanel->addWidget(m_pImageView_RectImage);
	pVBoxLayout_RightPanel->addWidget(m_pImageView_CircImage);

    pHBoxLayout->addItem(pVBoxLayout_RightPanel);

    this->setLayout(pHBoxLayout);


	// Connect signal and slot
	connect(this, SIGNAL(plotFringe(float*)), m_pScope_OctFringe, SLOT(drawData(float*)));
	connect(this, SIGNAL(plotAline(float*)), m_pScope_OctDepthProfile, SLOT(drawData(float*)));
	connect(this, SIGNAL(paintRectImage(uint8_t*)), m_pImageView_RectImage, SLOT(drawImage(uint8_t*)));
	connect(this, SIGNAL(paintCircImage(uint8_t*)), m_pImageView_CircImage, SLOT(drawImage(uint8_t*)));
	connect(m_pSlider_SelectAline, SIGNAL(valueChanged(int)), this, SLOT(updateAlinePos(int)));
}

QStreamTab::~QStreamTab()
{
	if (m_pImgObjRectImage) delete m_pImgObjRectImage;
	if (m_pImgObjCircImage) delete m_pImgObjCircImage;

	if (m_pMedfilt) delete m_pMedfilt;
	if (m_pCirc) delete m_pCirc;

	if (m_pThreadVisualization) delete m_pThreadVisualization;
	if (m_pThreadOctProcess) delete m_pThreadOctProcess;

	if (m_pOCT) delete m_pOCT;
}

void QStreamTab::keyPressEvent(QKeyEvent *e)
{
	if (e->key() != Qt::Key_Escape)
		QDialog::keyPressEvent(e);
}

void QStreamTab::setWidgetsText()
{
	m_pLineEdit_CircShift->setText(QString::number(m_pConfig->circShift));

	m_pComboBox_OctColorTable->setCurrentIndex(m_pConfig->octColorTable);
	m_pLineEdit_OctDbMax->setText(QString::number(m_pConfig->octDbRange.max));
	m_pLineEdit_OctDbMin->setText(QString::number(m_pConfig->octDbRange.min));
}


void QStreamTab::createOctVisualizationOptionTab()
{
    // Create widgets for OCT visualization option tab
    m_pGroupBox_OctVisualization = new QGroupBox;
	m_pGroupBox_OctVisualization->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    QGridLayout *pGridLayout_OctVisualization = new QGridLayout;
	pGridLayout_OctVisualization->setSpacing(3);

	// Create widgets for OCT calibration
	m_pPushButton_OctCalibration = new QPushButton(this);
	m_pPushButton_OctCalibration->setText("OCT Calibration...");

    // Create widgets for OCT visualization
    m_pCheckBox_CircularizeImage = new QCheckBox(this);
    m_pCheckBox_CircularizeImage->setText("Circularize Image");

    m_pLineEdit_CircShift = new QLineEdit(this);
	m_pLineEdit_CircShift->setFixedWidth(35);
	m_pLineEdit_CircShift->setText(QString("%1").arg(m_pConfig->circShift));
	m_pLineEdit_CircShift->setAlignment(Qt::AlignCenter);
    m_pLabel_CircShift = new QLabel("Circ Shift", this);
	m_pLabel_CircShift->setBuddy(m_pLineEdit_CircShift);

    // Create widgets for OCT color table
    m_pComboBox_OctColorTable = new QComboBox(this);
    m_pComboBox_OctColorTable->addItem("gray");
    m_pComboBox_OctColorTable->addItem("invgray");
	m_pComboBox_OctColorTable->addItem("sepia");
	m_pComboBox_OctColorTable->addItem("jet");
	m_pComboBox_OctColorTable->addItem("parula");
	m_pComboBox_OctColorTable->addItem("hot");
	m_pComboBox_OctColorTable->addItem("fire");
	
	m_pComboBox_OctColorTable->setCurrentIndex(m_pConfig->octColorTable);
    m_pLabel_OctColorTable = new QLabel("OCT Colortable", this);
    m_pLabel_OctColorTable->setBuddy(m_pComboBox_OctColorTable);

    // Create line edit widgets for OCT contrast adjustment
    m_pLineEdit_OctDbMax = new QLineEdit(this);
    m_pLineEdit_OctDbMax->setFixedWidth(30);
	m_pLineEdit_OctDbMax->setText(QString::number(m_pConfig->octDbRange.max));
	m_pLineEdit_OctDbMax->setAlignment(Qt::AlignCenter);
    m_pLineEdit_OctDbMin = new QLineEdit(this);
    m_pLineEdit_OctDbMin->setFixedWidth(30);
    m_pLineEdit_OctDbMin->setText(QString::number(m_pConfig->octDbRange.min));
	m_pLineEdit_OctDbMin->setAlignment(Qt::AlignCenter);

    // Create color bar for OCT visualization
    uint8_t color[256];
    for (int i = 0; i < 256; i++)
        color[i] = i;

    m_pImageView_OctDbColorbar = new QImageView(ColorTable::colortable(m_pConfig->octColorTable), 256, 1);
	m_pImageView_OctDbColorbar->setFixedHeight(15);
    m_pImageView_OctDbColorbar->drawImage(color);
    m_pLabel_OctDb = new QLabel("OCT dB", this);
    m_pLabel_OctDb->setFixedWidth(60);

    // Set layout
	pGridLayout_OctVisualization->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 0, 0);
	pGridLayout_OctVisualization->addWidget(m_pPushButton_OctCalibration, 0, 3, 1, 2);

	pGridLayout_OctVisualization->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 1, 0);
	pGridLayout_OctVisualization->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Fixed), 1, 1);
	pGridLayout_OctVisualization->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Fixed), 1, 2);
	pGridLayout_OctVisualization->addWidget(m_pLabel_OctColorTable, 1, 3);
	pGridLayout_OctVisualization->addWidget(m_pComboBox_OctColorTable, 1, 4);

	pGridLayout_OctVisualization->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 2, 0);
	pGridLayout_OctVisualization->addWidget(m_pCheckBox_CircularizeImage, 2, 1);
	pGridLayout_OctVisualization->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Fixed), 2, 2);
	pGridLayout_OctVisualization->addWidget(m_pLabel_CircShift, 2, 3);
	pGridLayout_OctVisualization->addWidget(m_pLineEdit_CircShift, 2, 4);

    QHBoxLayout *pHBoxLayout_OctDbColorbar = new QHBoxLayout;
    pHBoxLayout_OctDbColorbar->setSpacing(3);
    pHBoxLayout_OctDbColorbar->addWidget(m_pLabel_OctDb);
    pHBoxLayout_OctDbColorbar->addWidget(m_pLineEdit_OctDbMin);
    pHBoxLayout_OctDbColorbar->addWidget(m_pImageView_OctDbColorbar);
    pHBoxLayout_OctDbColorbar->addWidget(m_pLineEdit_OctDbMax);

	pGridLayout_OctVisualization->addItem(pHBoxLayout_OctDbColorbar, 3, 0, 1, 5);

    m_pGroupBox_OctVisualization->setLayout(pGridLayout_OctVisualization);
    
	// Connect signal and slot
	connect(m_pCheckBox_CircularizeImage, SIGNAL(toggled(bool)), this, SLOT(changeVisImage(bool)));	
	connect(m_pLineEdit_CircShift, SIGNAL(textEdited(const QString &)), this, SLOT(checkCircShift(const QString &)));
	connect(m_pComboBox_OctColorTable, SIGNAL(currentIndexChanged(int)), this, SLOT(changeOctColorTable(int)));
	connect(m_pLineEdit_OctDbMax, SIGNAL(textEdited(const QString &)), this, SLOT(adjustOctContrast()));
	connect(m_pLineEdit_OctDbMin, SIGNAL(textEdited(const QString &)), this, SLOT(adjustOctContrast()));

	connect(m_pPushButton_OctCalibration, SIGNAL(clicked(bool)), this, SLOT(createOctCalibDlg()));
}


void QStreamTab::setDataAcquisitionCallback()
{
#if NIIMAQ_ENABLE
	m_pDataAcq->ConnectFrameGrabberAcquiredData([&](int frame_count, const np::Array<uint16_t, 2>& frame) {

		// Data transfer		
		if (!(frame_count % RENEWAL_COUNT))
		{
			// Data transfer for data deinterleaving
			const uint16_t* frame_ptr = frame.raw_ptr();

			// Get buffer from threading queue
			uint16_t* raw_ptr = nullptr;
			{
				std::unique_lock<std::mutex> lock(m_syncOctProcessing.mtx);

				if (!m_syncOctProcessing.queue_buffer.empty())
				{
					raw_ptr = m_syncOctProcessing.queue_buffer.front();
					m_syncOctProcessing.queue_buffer.pop();
				}
			}

			if (raw_ptr != nullptr)
			{
				// Body
				int frame_length = m_pConfig->nFrameSize;
				memcpy(raw_ptr, frame_ptr, sizeof(uint16_t) * frame_length);

				// Push the buffer to sync Queue
				m_syncOctProcessing.Queue_sync.push(raw_ptr);
			}
		}

		// Buffering (When recording)
		if (m_pMemBuff->m_bIsRecording)
		{
			if (m_pMemBuff->m_nRecordedFrames < WRITING_BUFFER_SIZE)
			{
				// Get buffer from writing queue
				uint16_t* frame_ptr = nullptr;
				{
					std::unique_lock<std::mutex> lock(m_pMemBuff->m_syncBuffering.mtx);

					if (!m_pMemBuff->m_syncBuffering.queue_buffer.empty())
					{
						frame_ptr = m_pMemBuff->m_syncBuffering.queue_buffer.front();
						m_pMemBuff->m_syncBuffering.queue_buffer.pop();
					}
				}

				if (frame_ptr != nullptr)
				{
					// Body (Copying the frame data)
					memcpy(frame_ptr, frame.raw_ptr(), sizeof(uint16_t) * m_pConfig->nFrameSize);

					// Push to the copy queue for copying transfered data in copy thread
					m_pMemBuff->m_syncBuffering.Queue_sync.push(frame_ptr);
					m_pMemBuff->m_nRecordedFrames++;
				}
			}
			else
			{
				// Finish recording when the buffer is full
				m_pMemBuff->m_bIsRecording = false;
				m_pOperationTab->setRecordingButton(false);
			}
		}
	});

	m_pDataAcq->ConnectFrameGrabberStopData([&]() {
		m_syncOctProcessing.Queue_sync.push(nullptr);
	});

	m_pDataAcq->ConnectFrameGrabberSendStatusMessage([&](const char * msg) {
		std::thread t1([msg]()	{
			QMessageBox MsgBox(QMessageBox::Critical, "Error", msg);
			MsgBox.exec();
		});
		t1.detach();
	});
#endif
}

void QStreamTab::setOctProcessingCallback()
{
	// Ch1 Process Signal Objects ///////////////////////////////////////////////////////////////////////////////////////////
	m_pThreadOctProcess->DidAcquireData += [&](int frame_count) {

		// Get the buffer from the previous sync Queue
		uint16_t* fringe_data = m_syncOctProcessing.Queue_sync.pop();
		if (fringe_data != nullptr)
		{
			// Get buffers from threading queues
			float* res_ptr = nullptr;
			{
				std::unique_lock<std::mutex> lock(m_syncVisualization.mtx);

				if (!m_syncVisualization.queue_buffer.empty())
				{
					res_ptr = m_syncVisualization.queue_buffer.front();
					m_syncVisualization.queue_buffer.pop();
				}
			}

			if (res_ptr != nullptr)
            {
				// Body
				(*m_pOCT)(res_ptr, fringe_data);

				// Draw fringe
				ippsConvert_16u32f(fringe_data, m_visFringe.raw_ptr(), m_visFringe.length());
				emit plotFringe(&m_visFringe(0, m_pSlider_SelectAline->value()));

				// Transfer to OCT calibration dlg
				if (m_pOctCalibDlg)
					emit m_pOctCalibDlg->catchFringe(fringe_data);

				// Push the buffers to sync Queues
				m_syncVisualization.Queue_sync.push(res_ptr);

				// Return (push) the buffer to the previous threading queue
				{
					std::unique_lock<std::mutex> lock(m_syncOctProcessing.mtx);
					m_syncOctProcessing.queue_buffer.push(fringe_data);
				}
			}
		}
		else
			m_pThreadOctProcess->_running = false;

        (void)frame_count;
	};

	m_pThreadOctProcess->DidStopData += [&]() {
		m_syncVisualization.Queue_sync.push(nullptr);
	};

	m_pThreadOctProcess->SendStatusMessage += [&](const char* msg) {
		QMessageBox MsgBox(QMessageBox::Critical, "Error", msg);
		MsgBox.exec();
	};
}

void QStreamTab::setVisualizationCallback()
{
	// Visualization Signal Objects ///////////////////////////////////////////////////////////////////////////////////////////
	m_pThreadVisualization->DidAcquireData += [&](int frame_count) {

		// Get the buffers from the previous sync Queues
		float* res_data = m_syncVisualization.Queue_sync.pop();
		if (res_data != nullptr)
		{
			// Body	
			if (m_pOperationTab->isAcquisitionButtonToggled()) // Only valid if acquisition is running
            {
				// Draw A-lines
				m_visImage = np::FloatArray2(res_data, m_pConfig->n2ScansFFT, m_pConfig->nAlines);

				// Circ Shift
				for (int i = 0; i < m_pConfig->nAlines; i++)
				{
					float* pImg = m_visImage.raw_ptr() + i * m_pConfig->n2ScansFFT;
					std::rotate(pImg, pImg + (m_pConfig->n2ScansFFT - m_pConfig->circShift), pImg + m_pConfig->n2ScansFFT);
					memset(pImg, 0, sizeof(float) * m_pConfig->circShift);
				}

				emit plotAline(&m_visImage(0, m_pSlider_SelectAline->value()));

                // Draw Images
                visualizeImage(m_visImage.raw_ptr());
			}

			// Return (push) the buffer to the previous threading queue
			{
				std::unique_lock<std::mutex> lock(m_syncVisualization.mtx);
				m_syncVisualization.queue_buffer.push(res_data);
			}
		}
		else
		{
			if (res_data != nullptr)
			{
				float* res_temp = res_data;
				do
				{
					m_syncVisualization.queue_buffer.push(res_temp);
					res_temp = m_syncVisualization.Queue_sync.pop();
				} while (res_temp != nullptr);
			}

			m_pThreadVisualization->_running = false;

            (void)frame_count;
		}
	};

	m_pThreadVisualization->DidStopData += [&]() {
		//m_syncVisualization.Queue_sync.push(nullptr);
	};

	m_pThreadVisualization->SendStatusMessage += [&](const char* msg) {
		QMessageBox MsgBox(QMessageBox::Critical, "Error", msg);
		MsgBox.exec();
	};
}


void QStreamTab::resetObjectsForAline(int nAlines) // need modification
{	
	// Create data process object
	if (m_pOCT)
	{
		delete m_pOCT;
		m_pOCT = new OCTProcess(m_pConfig->nScans, nAlines);
		m_pOCT->loadCalibration();
	}

	// Create buffers for threading operation
	m_pMemBuff->m_syncBuffering.deallocate_queue_buffer();
	m_syncOctProcessing.deallocate_queue_buffer();
	m_syncVisualization.deallocate_queue_buffer();

    m_pMemBuff->m_syncBuffering.allocate_queue_buffer(m_pConfig->nScans, m_pConfig->nAlines, PROCESSING_BUFFER_SIZE);
    m_syncOctProcessing.allocate_queue_buffer(m_pConfig->nScans, nAlines, PROCESSING_BUFFER_SIZE);
    m_syncVisualization.allocate_queue_buffer(m_pConfig->n2ScansFFT, nAlines, PROCESSING_BUFFER_SIZE);

	// Reset rect image size
	m_pImageView_RectImage->resetSize(nAlines, m_pConfig->n2ScansFFT);

	// Reset scan adjust range
#ifdef GALVANO_MIRROR
	m_pMainWnd->m_pDeviceControlTab->setScrollBarRange(nAlines);
#endif
	
	// Create visualization buffers
	m_visFringe = np::FloatArray2(m_pConfig->nScans, nAlines);
	m_visImage = np::FloatArray2(m_pConfig->n2ScansFFT, nAlines);

	// Create image visualization buffers
	ColorTable temp_ctable;
	if (m_pImgObjRectImage) delete m_pImgObjRectImage;
	m_pImgObjRectImage = new ImageObject(nAlines, m_pConfig->n2ScansFFT, temp_ctable.m_colorTableVector.at(m_pComboBox_OctColorTable->currentIndex()));


	// Create circularize object
	if (m_pCirc)
	{
		delete m_pCirc;
		m_pCirc = new circularize(CIRC_RADIUS, nAlines, false);
	}
	if (m_pMedfilt)
	{
		delete m_pMedfilt;
		m_pMedfilt = new medfilt(nAlines, m_pConfig->n2ScansFFT, 3, 3);
	}

	// Reset slider range
	m_pSlider_SelectAline->setRange(0, nAlines - 1);

	// Reset slider label
	QString str; str.sprintf("Current A-line : %4d / %4d   ", 1, nAlines);
	m_pLabel_SelectAline->setText(str);
}

void QStreamTab::visualizeImage(float* res)
{
	IppiSize roi_oct = { m_pConfig->n2ScansFFT, m_pConfig->nAlines };
	
	// OCT Visualization
	np::Uint8Array2 scale_temp(roi_oct.width, roi_oct.height);
	ippiScale_32f8u_C1R(res, roi_oct.width * sizeof(float), scale_temp.raw_ptr(), roi_oct.width * sizeof(uint8_t), roi_oct, m_pConfig->octDbRange.min, m_pConfig->octDbRange.max);
	ippiTranspose_8u_C1R(scale_temp.raw_ptr(), roi_oct.width * sizeof(uint8_t), m_pImgObjRectImage->arr.raw_ptr(), roi_oct.height * sizeof(uint8_t), roi_oct);
#ifdef GALVANO_MIRROR
	if (m_pConfig->galvoHorizontalShift)
	{
		for (int i = 0; i < m_pConfig->n2ScansFFT; i++)
		{
			uint8_t* pImg = m_pImgObjRectImage->arr.raw_ptr() + i * m_pConfig->nAlines;
			std::rotate(pImg, pImg + m_pConfig->galvoHorizontalShift, pImg + m_pConfig->nAlines);
		}
	}
#endif
	(*m_pMedfilt)(m_pImgObjRectImage->arr.raw_ptr());	


	if (!m_pCheckBox_CircularizeImage->isChecked()) // rect image 
	{        
		emit paintRectImage(m_pImgObjRectImage->arr.raw_ptr());
	}
	else // circ image
    {
		(*m_pCirc)(m_pImgObjRectImage->arr, m_pImgObjCircImage->arr.raw_ptr(), "vertical", 0);
		emit paintCircImage(m_pImgObjCircImage->qindeximg.bits());
	}
}


void QStreamTab::updateAlinePos(int aline)
{
	// Reset channel data
	if (!m_pOperationTab->isAcquisitionButtonToggled())
	{
		emit plotFringe(&m_visFringe(0, m_pSlider_SelectAline->value()));
		emit plotAline(&m_visImage(0, m_pSlider_SelectAline->value()));
	}

	// Reset slider label
	QString str; str.sprintf("Current A-line : %4d / %4d   ", aline + 1, m_pConfig->nAlines);
	m_pLabel_SelectAline->setText(str);
}

void QStreamTab::changeVisImage(bool toggled)
{	
	if (toggled)
	{
		m_pImageView_CircImage->show();// setVisible(toggled);
		m_pImageView_RectImage->hide();// setVisible(!toggled);
	}
	else
	{
		m_pImageView_CircImage->hide(); // setVisible(toggled);
		m_pImageView_RectImage->show(); // etVisible(!toggled);
	}

	if (!m_pOperationTab->isAcquisitionButtonToggled())
		visualizeImage(m_visImage.raw_ptr());
}

void QStreamTab::checkCircShift(const QString &str)
{
	int circShift = str.toInt();
	if (circShift > CIRC_RADIUS - PROJECTION_OFFSET)
	{
		circShift = CIRC_RADIUS - PROJECTION_OFFSET;
		m_pLineEdit_CircShift->setText(QString::number(circShift));
	}
	m_pConfig->circShift = circShift;

	if (!m_pOperationTab->isAcquisitionButtonToggled())
		visualizeImage(m_visImage.raw_ptr());
}

void QStreamTab::changeOctColorTable(int ctable_ind)
{
	m_pConfig->octColorTable = ctable_ind;

	m_pImageView_RectImage->resetColormap(ColorTable::colortable(ctable_ind));
	m_pImageView_CircImage->resetColormap(ColorTable::colortable(ctable_ind));
	m_pImageView_OctDbColorbar->resetColormap(ColorTable::colortable(ctable_ind));

	ColorTable temp_ctable;
	if (m_pImgObjRectImage) delete m_pImgObjRectImage;
	m_pImgObjRectImage = new ImageObject(m_pImageView_RectImage->getRender()->m_pImage->width(), m_pImageView_RectImage->getRender()->m_pImage->height(), temp_ctable.m_colorTableVector.at(ctable_ind));
	if (m_pImgObjCircImage) delete m_pImgObjCircImage;
	m_pImgObjCircImage = new ImageObject(m_pImageView_CircImage->getRender()->m_pImage->width(), m_pImageView_CircImage->getRender()->m_pImage->height(), temp_ctable.m_colorTableVector.at(ctable_ind));

	if (!m_pOperationTab->isAcquisitionButtonToggled())	
		visualizeImage(m_visImage.raw_ptr());
}

void QStreamTab::adjustOctContrast()
{
	int min_dB = m_pLineEdit_OctDbMin->text().toInt();
	int max_dB = m_pLineEdit_OctDbMax->text().toInt();
			
	m_pConfig->octDbRange.min = min_dB;
	m_pConfig->octDbRange.max = max_dB;

	m_pScope_OctDepthProfile->resetAxis({ 0, (double)m_pConfig->n2ScansFFT }, { (double)min_dB, (double)max_dB }, 1, 1, 0, 0, "", "dB");
	if (m_pOctCalibDlg != nullptr)
		m_pOctCalibDlg->m_pScope->resetAxis({ 0, (double)m_pConfig->n2ScansFFT }, { (double)min_dB, (double)max_dB });

	if (!m_pOperationTab->isAcquisitionButtonToggled())	
	{
		emit plotAline(&m_visImage(0, m_pSlider_SelectAline->value()));
		visualizeImage(m_visImage.raw_ptr());
	}
}

void QStreamTab::createOctCalibDlg()
{			
    if (m_pOctCalibDlg == nullptr)
    {
        m_pOctCalibDlg = new OctCalibDlg(this);
        connect(m_pOctCalibDlg, SIGNAL(finished(int)), this, SLOT(deleteOctCalibDlg()));
        m_pOctCalibDlg->show();
    }
    m_pOctCalibDlg->raise();
    m_pOctCalibDlg->activateWindow();
}

void QStreamTab::deleteOctCalibDlg()
{
	m_pOctCalibDlg->deleteLater();
	m_pOctCalibDlg = nullptr;
}
