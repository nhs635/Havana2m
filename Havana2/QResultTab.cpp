
#include "QResultTab.h"

#include <Havana2/MainWindow.h>
#include <Havana2/QOperationTab.h>
#include <Havana2/QStreamTab.h>
#ifdef GALVANO_MIRROR
#include <Havana2/QDeviceControlTab.h>
#endif
#include <Havana2/Viewer/QImageView.h>
#include <Havana2/Dialog/SaveResultDlg.h>

#include <MemoryBuffer/MemoryBuffer.h>

#include <DataProcess/OCTProcess/OCTProcess.h>

#include <ipps.h>
#include <ippi.h>
#include <ippcc.h>
#include <ippvm.h>

#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>

#include <iostream>
#include <thread>

#include <time.h>

#define IN_BUFFER_DATA -2
#define EXTERNAL_DATA  -3


QResultTab::QResultTab(QWidget *parent) :
    QDialog(parent), 
	m_pImgObjRectImage(nullptr), m_pImgObjCircImage(nullptr), m_pCirc(nullptr),
	m_pMedfiltRect(nullptr), 
	m_pSaveResultDlg(nullptr)
{
	// Set main window objects
	m_pMainWnd = (MainWindow*)parent;
	m_pConfig = m_pMainWnd->m_pConfiguration;
#ifdef GALVANO_MIRROR
	m_pDeviceControlTab = m_pMainWnd->m_pDeviceControlTab;
#endif
	m_pMemBuff = m_pMainWnd->m_pOperationTab->m_pMemoryBuffer;


    // Create layout
	QHBoxLayout* pHBoxLayout = new QHBoxLayout;
	pHBoxLayout->setSpacing(0);

    // Create image view
	QVBoxLayout* pVBoxLayout_ImageView = new QVBoxLayout;
	pVBoxLayout_ImageView->setSpacing(0);

    m_pImageView_RectImage = new QImageView(ColorTable::colortable(m_pConfig->octColorTable), m_pConfig->nAlines, m_pConfig->nScansFFT);
    m_pImageView_RectImage->setMinimumWidth(600);
	m_pImageView_RectImage->setDisabled(true);
	m_pImageView_RectImage->setMovedMouseCallback([&](QPoint& p) { m_pMainWnd->m_pStatusLabel_ImagePos->setText(QString("(%1, %2)").arg(p.x(), 4).arg(p.y(), 4)); });
	m_pImageView_RectImage->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    m_pImageView_CircImage = new QImageView(ColorTable::colortable(m_pConfig->octColorTable), 2 * CIRC_RADIUS, 2 * CIRC_RADIUS);
    m_pImageView_CircImage->setMinimumWidth(600);
	m_pImageView_CircImage->setMinimumHeight(600);
	m_pImageView_CircImage->setDisabled(true);
	m_pImageView_CircImage->setMovedMouseCallback([&](QPoint& p) { m_pMainWnd->m_pStatusLabel_ImagePos->setText(QString("(%1, %2)").arg(p.x(), 4).arg(p.y(), 4)); });
	m_pImageView_CircImage->setSquare(true);
	m_pImageView_CircImage->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	m_pImageView_CircImage->setVisible(false);

	QLabel *pNullLabel = new QLabel("", this);
	pNullLabel->setMinimumWidth(600);
	pNullLabel->setFixedHeight(0);
	pNullLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

	// Create image view buffers
	ColorTable temp_ctable;
	m_pImgObjRectImage = new ImageObject(m_pConfig->nAlines4, m_pConfig->n2ScansFFT, temp_ctable.m_colorTableVector.at(m_pConfig->octColorTable));
	m_pImgObjCircImage = new ImageObject(2 * CIRC_RADIUS, 2 * CIRC_RADIUS, temp_ctable.m_colorTableVector.at(m_pConfig->octColorTable));

    // Set layout for left panel
	pVBoxLayout_ImageView->addWidget(m_pImageView_RectImage);
	pVBoxLayout_ImageView->addWidget(m_pImageView_CircImage);
	pVBoxLayout_ImageView->addWidget(pNullLabel);

	pHBoxLayout->addItem(pVBoxLayout_ImageView);
	
    // Create data loading & writing tab
    createDataLoadingWritingTab();

    // Create visualization option tab
    createVisualizationOptionTab();

    // Create en face map tab
    createEnFaceMapTab();

    // Set layout for right panel
    QVBoxLayout *pVBoxLayout_RightPanel = new QVBoxLayout;
    pVBoxLayout_RightPanel->setSpacing(0);
	pVBoxLayout_RightPanel->setContentsMargins(0, 0, 0, 0);

    pVBoxLayout_RightPanel->addWidget(m_pGroupBox_DataLoadingWriting);
	pVBoxLayout_RightPanel->addWidget(m_pGroupBox_Visualization);
    pVBoxLayout_RightPanel->addWidget(m_pGroupBox_EnFace);

    pHBoxLayout->addItem(pVBoxLayout_RightPanel);

    this->setLayout(pHBoxLayout);
	
	// Connect signal and slot
	connect(this, SIGNAL(setWidgets(bool, Configuration*)), this, SLOT(setWidgetsEnabled(bool, Configuration*)));
	connect(this, SIGNAL(paintRectImage(uint8_t*)), m_pImageView_RectImage, SLOT(drawImage(uint8_t*)));
	connect(this, SIGNAL(paintCircImage(uint8_t*)), m_pImageView_CircImage, SLOT(drawImage(uint8_t*)));
}

QResultTab::~QResultTab()
{
	if (m_pImgObjRectImage) delete m_pImgObjRectImage;
	if (m_pImgObjCircImage) delete m_pImgObjCircImage;

	if (m_pMedfiltRect) delete m_pMedfiltRect;
	if (m_pCirc) delete m_pCirc;	
}

void QResultTab::keyPressEvent(QKeyEvent *e)
{
	if (e->key() != Qt::Key_Escape)
		QDialog::keyPressEvent(e);
}

void QResultTab::setWidgetsText()
{
	m_pLineEdit_CircShift->setText(QString::number(m_pConfig->circShift));

	m_pComboBox_OctColorTable->setCurrentIndex(m_pConfig->octColorTable);
	m_pLineEdit_OctDbMax->setText(QString::number(m_pConfig->octDbRange.max));
	m_pLineEdit_OctDbMin->setText(QString::number(m_pConfig->octDbRange.min));
}


void QResultTab::createDataLoadingWritingTab()
{
    // Create widgets for loading and writing tab
    m_pGroupBox_DataLoadingWriting = new QGroupBox;
	m_pGroupBox_DataLoadingWriting->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    QGridLayout *pGridLayout_DataLoadingWriting = new QGridLayout;
    pGridLayout_DataLoadingWriting->setSpacing(3);
	
    // Create radio buttons to chooose data for the processing
	m_pRadioButton_InBuffer = new QRadioButton(this);
    m_pRadioButton_InBuffer->setText("Use In-Buffer Data  ");
	m_pRadioButton_InBuffer->setDisabled(true);
    m_pRadioButton_External = new QRadioButton(this);
    m_pRadioButton_External->setText("Use External Data  ");

	m_pButtonGroup_DataSelection = new QButtonGroup(this);
	m_pButtonGroup_DataSelection->addButton(m_pRadioButton_InBuffer, IN_BUFFER_DATA);
	m_pButtonGroup_DataSelection->addButton(m_pRadioButton_External, EXTERNAL_DATA);
	m_pRadioButton_External->setChecked(true);

    // Create buttons for the processing
    m_pPushButton_StartProcessing = new QPushButton(this);
	m_pPushButton_StartProcessing->setText("Start Processing");
	m_pPushButton_StartProcessing->setFixedWidth(130);

    m_pPushButton_SaveResults = new QPushButton(this);
	m_pPushButton_SaveResults->setText("Save Results...");
	m_pPushButton_SaveResults->setFixedWidth(130);
	m_pPushButton_SaveResults->setDisabled(true);

	// Create widgets for user defined parameter 
	m_pCheckBox_UserDefinedAlines = new QCheckBox(this);
	m_pCheckBox_UserDefinedAlines->setText("User-Defined nAlines");

	m_pLineEdit_UserDefinedAlines = new QLineEdit(this);
	m_pLineEdit_UserDefinedAlines->setText(QString::number(m_pConfig->nAlines));
	m_pLineEdit_UserDefinedAlines->setFixedWidth(35);
	m_pLineEdit_UserDefinedAlines->setAlignment(Qt::AlignCenter);
	m_pLineEdit_UserDefinedAlines->setDisabled(true);
	
	m_pCheckBox_SingleFrame = new QCheckBox(this);
	m_pCheckBox_SingleFrame->setText("Single Frame Processing");

	m_pLabel_DiscomValue = new QLabel("    Discom Value", this);

	m_pLineEdit_DiscomValue = new QLineEdit(this);
	m_pLineEdit_DiscomValue->setText(QString::number(m_pConfig->octDiscomVal));
	m_pLineEdit_DiscomValue->setFixedWidth(30);
	m_pLineEdit_DiscomValue->setAlignment(Qt::AlignCenter);

	// Create progress bar
	m_pProgressBar_PostProcessing = new QProgressBar(this);
	m_pProgressBar_PostProcessing->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	m_pProgressBar_PostProcessing->setFormat("");
	m_pProgressBar_PostProcessing->setValue(0);
	
    // Set layout
	pGridLayout_DataLoadingWriting->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 0, 0, 3, 1);

	pGridLayout_DataLoadingWriting->addWidget(m_pRadioButton_InBuffer, 0, 1);
	pGridLayout_DataLoadingWriting->addWidget(m_pRadioButton_External, 1, 1);

	pGridLayout_DataLoadingWriting->addWidget(m_pPushButton_StartProcessing, 0, 2);
	pGridLayout_DataLoadingWriting->addWidget(m_pPushButton_SaveResults, 1, 2);
	
	QHBoxLayout *pHBoxLayout_UserDefined = new QHBoxLayout;
	pHBoxLayout_UserDefined->setSpacing(3);

	pHBoxLayout_UserDefined->addWidget(m_pCheckBox_UserDefinedAlines);
	pHBoxLayout_UserDefined->addWidget(m_pLineEdit_UserDefinedAlines);
	pHBoxLayout_UserDefined->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
	
	QHBoxLayout *pHBoxLayout_SingleFrameDiscomValue = new QHBoxLayout;
	pHBoxLayout_SingleFrameDiscomValue->setSpacing(3);

	pHBoxLayout_SingleFrameDiscomValue->addWidget(m_pCheckBox_SingleFrame);
	pHBoxLayout_SingleFrameDiscomValue->addWidget(m_pLabel_DiscomValue);
	pHBoxLayout_SingleFrameDiscomValue->addWidget(m_pLineEdit_DiscomValue);

	pGridLayout_DataLoadingWriting->addItem(pHBoxLayout_UserDefined, 2, 1, 1, 2);
	pGridLayout_DataLoadingWriting->addItem(pHBoxLayout_SingleFrameDiscomValue, 3, 1, 1, 2);

	pGridLayout_DataLoadingWriting->addWidget(m_pProgressBar_PostProcessing, 4, 1, 1, 2);

	pGridLayout_DataLoadingWriting->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 0, 3, 3, 1);

    m_pGroupBox_DataLoadingWriting->setLayout(pGridLayout_DataLoadingWriting);

	// Connect signal and slot
	connect(m_pButtonGroup_DataSelection, SIGNAL(buttonClicked(int)), this, SLOT(changeDataSelection(int)));
	connect(m_pPushButton_StartProcessing, SIGNAL(clicked(bool)), this, SLOT(startProcessing(void)));
	connect(m_pPushButton_SaveResults, SIGNAL(clicked(bool)), this, SLOT(createSaveResultDlg()));
	connect(m_pCheckBox_UserDefinedAlines, SIGNAL(toggled(bool)), this, SLOT(enableUserDefinedAlines(bool)));

	connect(this, SIGNAL(processedSingleFrame(int)), m_pProgressBar_PostProcessing, SLOT(setValue(int)));
}

void QResultTab::createVisualizationOptionTab()
{
    // Create widgets for visualization option tab
    m_pGroupBox_Visualization = new QGroupBox;
	m_pGroupBox_Visualization->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    QGridLayout *pGridLayout_Visualization = new QGridLayout;    
	pGridLayout_Visualization->setSpacing(3);

    // Create slider for exploring frames
    m_pSlider_SelectFrame = new QSlider(this);
    m_pSlider_SelectFrame->setOrientation(Qt::Horizontal);
    m_pSlider_SelectFrame->setValue(0);
	m_pSlider_SelectFrame->setDisabled(true);

    m_pLabel_SelectFrame = new QLabel(this);
    QString str; str.sprintf("Current Frame : %3d / %3d", 1, 1);
    m_pLabel_SelectFrame->setText(str);
	m_pLabel_SelectFrame->setFixedWidth(130);
    m_pLabel_SelectFrame->setBuddy(m_pSlider_SelectFrame);
	m_pLabel_SelectFrame->setDisabled(true);

    // Create push button for measuring distance
    m_pToggleButton_MeasureDistance = new QPushButton(this);
	m_pToggleButton_MeasureDistance->setCheckable(true);
    m_pToggleButton_MeasureDistance->setText("Measure Distance");
	m_pToggleButton_MeasureDistance->setDisabled(true);
	
    // Create checkboxs for OCT operation
    m_pCheckBox_ShowGuideLine = new QCheckBox(this);
    m_pCheckBox_ShowGuideLine->setText("Show Guide Line");
	m_pCheckBox_ShowGuideLine->setDisabled(true);
    m_pCheckBox_CircularizeImage = new QCheckBox(this);
    m_pCheckBox_CircularizeImage->setText("Circularize Image");
	m_pCheckBox_CircularizeImage->setDisabled(true);

    // Create widgets for OCT circularizing
    m_pLineEdit_CircShift = new QLineEdit(this);
	m_pLineEdit_CircShift->setFixedWidth(30);
	m_pLineEdit_CircShift->setText(QString::number(m_pConfig->circShift));
	m_pLineEdit_CircShift->setAlignment(Qt::AlignCenter);
	m_pLineEdit_CircShift->setDisabled(true);
    m_pLabel_CircShift = new QLabel("Circ Shift", this);
	m_pLabel_CircShift->setBuddy(m_pLineEdit_CircShift);
	m_pLabel_CircShift->setDisabled(true);

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
    m_pComboBox_OctColorTable->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	m_pComboBox_OctColorTable->setDisabled(true);
    m_pLabel_OctColorTable = new QLabel("OCT Colortable", this);
    m_pLabel_OctColorTable->setBuddy(m_pComboBox_OctColorTable);
	m_pLabel_OctColorTable->setDisabled(true);

    // Set layout
	pGridLayout_Visualization->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 0, 0);
	pGridLayout_Visualization->addWidget(m_pLabel_SelectFrame, 0, 1);
    pGridLayout_Visualization->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 0, 2);
    pGridLayout_Visualization->addWidget(m_pToggleButton_MeasureDistance, 0, 3, 1, 2);
    pGridLayout_Visualization->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 0, 5);

    pGridLayout_Visualization->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 1, 0);
    pGridLayout_Visualization->addWidget(m_pSlider_SelectFrame, 1, 1, 1, 4);
    pGridLayout_Visualization->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 1, 5);

    pGridLayout_Visualization->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 2, 0);
    pGridLayout_Visualization->addWidget(m_pCheckBox_CircularizeImage, 2, 1);
    pGridLayout_Visualization->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 2, 2);
    pGridLayout_Visualization->addWidget(m_pLabel_CircShift, 2, 3);
    pGridLayout_Visualization->addWidget(m_pLineEdit_CircShift, 2, 4);
    pGridLayout_Visualization->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 2, 5);

    pGridLayout_Visualization->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 3, 0);
    pGridLayout_Visualization->addWidget(m_pCheckBox_ShowGuideLine, 3, 1);
    pGridLayout_Visualization->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 3, 2);
    pGridLayout_Visualization->addWidget(m_pLabel_OctColorTable, 3, 3);
    pGridLayout_Visualization->addWidget(m_pComboBox_OctColorTable, 3, 4);
    pGridLayout_Visualization->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed), 3, 5);

    m_pGroupBox_Visualization->setLayout(pGridLayout_Visualization);

	// Connect signal and slot
	connect(m_pSlider_SelectFrame, SIGNAL(valueChanged(int)), this, SLOT(visualizeImage(int)));
	connect(m_pToggleButton_MeasureDistance, SIGNAL(toggled(bool)), this, SLOT(measureDistance(bool)));
	connect(m_pCheckBox_ShowGuideLine, SIGNAL(toggled(bool)), this, SLOT(showGuideLine(bool)));
	connect(m_pCheckBox_CircularizeImage, SIGNAL(toggled(bool)), this, SLOT(changeVisImage(bool)));
	connect(m_pLineEdit_CircShift, SIGNAL(textEdited(const QString &)), this, SLOT(checkCircShift(const QString &)));
	connect(m_pComboBox_OctColorTable, SIGNAL(currentIndexChanged(int)), this, SLOT(adjustOctContrast()));
}

void QResultTab::createEnFaceMapTab()
{    
    // En face map tab widgets
	m_pGroupBox_EnFace = new QGroupBox;
	m_pGroupBox_EnFace->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QGridLayout *pGridLayout_EnFace = new QGridLayout;
    pGridLayout_EnFace->setSpacing(0);

    uint8_t color[256 * 4];
    for (int i = 0; i < 256 * 4; i++)
        color[i] = 255 - i / 4;

    // Create widgets for OCT projection map
    m_pImageView_OctProjection = new QImageView(ColorTable::colortable(m_pConfig->octColorTable), m_pConfig->nAlines, 1);
#ifdef OCT_NIRF
	m_pImageView_OctProjection->setMinimumHeight(150);
#endif
	m_pImageView_OctProjection->setHLineChangeCallback([&](int frame) { m_pSlider_SelectFrame->setValue(frame); });
	m_visOctProjection = np::Uint8Array2(m_pConfig->nAlines4, 1);

    m_pLineEdit_OctDbMax = new QLineEdit(this);
    m_pLineEdit_OctDbMax->setText(QString::number(m_pConfig->octDbRange.max));
	m_pLineEdit_OctDbMax->setAlignment(Qt::AlignCenter);
    m_pLineEdit_OctDbMax->setFixedWidth(30);
	m_pLineEdit_OctDbMax->setDisabled(true);
    m_pLineEdit_OctDbMin = new QLineEdit(this);
    m_pLineEdit_OctDbMin->setText(QString::number(m_pConfig->octDbRange.min));
	m_pLineEdit_OctDbMin->setAlignment(Qt::AlignCenter);
    m_pLineEdit_OctDbMin->setFixedWidth(30);
	m_pLineEdit_OctDbMin->setDisabled(true);

    m_pImageView_ColorbarOctProjection = new QImageView(ColorTable::colortable(m_pConfig->octColorTable), 4, 256);
	m_pImageView_ColorbarOctProjection->getRender()->setFixedWidth(15);
    m_pImageView_ColorbarOctProjection->drawImage(color);
    m_pImageView_ColorbarOctProjection->setFixedWidth(30);

    m_pLabel_OctProjection = new QLabel(this);
    m_pLabel_OctProjection->setText("OCT Maximum Projection Map");

    // Set layout
    pGridLayout_EnFace->addWidget(m_pLabel_OctProjection, 0, 0, 1, 3);
    pGridLayout_EnFace->addWidget(m_pImageView_OctProjection, 1, 0);
    pGridLayout_EnFace->addWidget(m_pImageView_ColorbarOctProjection, 1, 1);
    QVBoxLayout *pVBoxLayout_Colorbar1 = new QVBoxLayout;
	pVBoxLayout_Colorbar1->setSpacing(0);
    pVBoxLayout_Colorbar1->addWidget(m_pLineEdit_OctDbMax);
    pVBoxLayout_Colorbar1->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding));
    pVBoxLayout_Colorbar1->addWidget(m_pLineEdit_OctDbMin);
    pGridLayout_EnFace->addItem(pVBoxLayout_Colorbar1, 1, 2);
	
    m_pGroupBox_EnFace->setLayout(pGridLayout_EnFace);

	// Connect signal and slot
	connect(this, SIGNAL(paintOctProjection(uint8_t*)), m_pImageView_OctProjection, SLOT(drawImage(uint8_t*)));
	connect(m_pLineEdit_OctDbMax, SIGNAL(textEdited(const QString &)), this, SLOT(adjustOctContrast()));
	connect(m_pLineEdit_OctDbMin, SIGNAL(textEdited(const QString &)), this, SLOT(adjustOctContrast()));
}


void QResultTab::changeDataSelection(int id)
{
	switch (id)
	{
	case IN_BUFFER_DATA:
		m_pCheckBox_UserDefinedAlines->setDisabled(true);
		if (m_pCheckBox_UserDefinedAlines->isChecked())
			m_pCheckBox_UserDefinedAlines->setChecked(false);
		break;
	case EXTERNAL_DATA:
		m_pCheckBox_UserDefinedAlines->setEnabled(true);
		break;
	}	
}

void QResultTab::createSaveResultDlg()
{
	if (m_pSaveResultDlg == nullptr)
	{
		m_pSaveResultDlg = new SaveResultDlg(this);
		connect(m_pSaveResultDlg, SIGNAL(finished(int)), this, SLOT(deleteSaveResultDlg()));
		m_pSaveResultDlg->show();
	}
	m_pSaveResultDlg->raise();
	m_pSaveResultDlg->activateWindow();
}

void QResultTab::deleteSaveResultDlg()
{
	m_pSaveResultDlg->deleteLater();
	m_pSaveResultDlg = nullptr;
}

void QResultTab::enableUserDefinedAlines(bool checked)
{ 
	m_pLineEdit_UserDefinedAlines->setEnabled(checked); 
}

void QResultTab::visualizeImage(int frame)
{
	if (m_vectorOctImage.size() != 0)
	{
		IppiSize roi_oct = { m_pImgObjRectImage->getHeight(), m_pImgObjRectImage->getWidth() };

		// OCT Visualization
		np::Uint8Array2 scale_temp(roi_oct.width, roi_oct.height);
		ippiScale_32f8u_C1R(m_vectorOctImage.at(frame), roi_oct.width * sizeof(float),
			scale_temp.raw_ptr(), roi_oct.width * sizeof(uint8_t), roi_oct, m_pConfig->octDbRange.min, m_pConfig->octDbRange.max);

		for (int i = 0; i < roi_oct.height; i++)
		{
			uint8_t* pImg = scale_temp.raw_ptr() + i * roi_oct.width;
			std::rotate(pImg, pImg + (roi_oct.width - m_pConfig->circShift), pImg + roi_oct.width);
			memset(pImg, 0, sizeof(uint8_t) * m_pConfig->circShift);
		}

		ippiTranspose_8u_C1R(scale_temp.raw_ptr(), roi_oct.width * sizeof(uint8_t), m_pImgObjRectImage->arr.raw_ptr(), roi_oct.height * sizeof(uint8_t), roi_oct);
#ifdef GALVANO_MIRROR
		if (m_pConfig->galvoHorizontalShift)
		{
			for (int i = 0; i < roi_oct.width; i++)
			{
				uint8_t* pImg = m_pImgObjRectImage->arr.raw_ptr() + i * roi_oct.height;
				std::rotate(pImg, pImg + m_pConfig->galvoHorizontalShift, pImg + roi_oct.height);
			}
		}
#endif
		(*m_pMedfiltRect)(m_pImgObjRectImage->arr.raw_ptr());

		if (!m_pCheckBox_CircularizeImage->isChecked())
		{
			if (m_pImageView_RectImage->isEnabled()) emit paintRectImage(m_pImgObjRectImage->qindeximg.bits());
		}
		else
		{
			(*m_pCirc)(m_pImgObjRectImage->arr, m_pImgObjCircImage->arr.raw_ptr(), "vertical", 0);
			if (m_pImageView_CircImage->isEnabled()) emit paintCircImage(m_pImgObjCircImage->qindeximg.bits());
		}

		m_pImageView_OctProjection->setHorizontalLine(1, m_visOctProjection.size(1) - frame);
		m_pImageView_OctProjection->getRender()->update();

		QString str; str.sprintf("Current Frame : %3d / %3d", frame + 1, (int)m_vectorOctImage.size());
		m_pLabel_SelectFrame->setText(str);
	}
}

void QResultTab::visualizeEnFaceMap(bool scaling)
{
	if (m_octProjection.size(0) != 0)
	{
		if (scaling)
		{
			IppiSize roi_proj = { m_octProjection.size(0), m_octProjection.size(1) };

			// Scaling OCT projection
			ippiScale_32f8u_C1R(m_octProjection, sizeof(float) * roi_proj.width, m_visOctProjection, sizeof(uint8_t) * roi_proj.width,
				roi_proj, m_pConfig->octDbRange.min, m_pConfig->octDbRange.max);
			ippiMirror_8u_C1IR(m_visOctProjection, sizeof(uint8_t) * roi_proj.width, roi_proj, ippAxsHorizontal);
#ifdef GALVANO_MIRROR
			if (m_pConfig->galvoHorizontalShift)
			{
				for (int i = 0; i < roi_proj.height; i++)
				{
					uint8_t* pImg = m_visOctProjection.raw_ptr() + i * roi_proj.width;
					std::rotate(pImg, pImg + m_pConfig->galvoHorizontalShift, pImg + roi_proj.width);
				}
			}
#endif
		}
		emit paintOctProjection(m_visOctProjection);
	}
}

void QResultTab::measureDistance(bool toggled)
{
	m_pImageView_CircImage->getRender()->m_bMeasureDistance = toggled;
	if (!toggled)
	{
		m_pImageView_CircImage->getRender()->m_nClicked = 0;
		m_pImageView_CircImage->getRender()->update();
	}
}

void QResultTab::showGuideLine(bool toggled)
{
	if (toggled)
	{
		m_pImageView_RectImage->setHorizontalLine(3, m_pConfig->circShift, m_pConfig->circShift + PROJECTION_OFFSET, CIRC_RADIUS);
		m_pImageView_CircImage->setCircle(2, m_pConfig->circShift, m_pConfig->circShift + PROJECTION_OFFSET);
	}
	else
	{
		m_pImageView_RectImage->setHorizontalLine(0);
		m_pImageView_CircImage->setCircle(0);
	}

	m_pImageView_RectImage->getRender()->update();
	m_pImageView_CircImage->getRender()->update();
}

void QResultTab::changeVisImage(bool toggled)
{	
	if (toggled) // circ view
	{
		m_pImageView_CircImage->setVisible(toggled);
		m_pImageView_RectImage->setVisible(!toggled);
	}
	else // rect view
	{
		m_pToggleButton_MeasureDistance->setChecked(false);
		m_pImageView_CircImage->setVisible(toggled);
		m_pImageView_RectImage->setVisible(!toggled);
	}
	visualizeImage(m_pSlider_SelectFrame->value());

	m_pToggleButton_MeasureDistance->setEnabled(toggled);
}

void QResultTab::checkCircShift(const QString &str)
{
	int circShift = str.toInt();
	if (circShift > CIRC_RADIUS - PROJECTION_OFFSET)
	{
		circShift = CIRC_RADIUS - PROJECTION_OFFSET;
		m_pLineEdit_CircShift->setText(QString("%1").arg(circShift));
	}
	m_pConfig->circShift = circShift;

	if (m_pCheckBox_ShowGuideLine->isChecked())
	{
		m_pImageView_RectImage->setHorizontalLine(3, m_pConfig->circShift, m_pConfig->circShift + PROJECTION_OFFSET, CIRC_RADIUS);
		m_pImageView_CircImage->setCircle(2, m_pConfig->circShift, m_pConfig->circShift + PROJECTION_OFFSET);
	}

	getOctProjection(m_vectorOctImage, m_octProjection, circShift);
	visualizeEnFaceMap(true);
	visualizeImage(m_pSlider_SelectFrame->value());
}

void QResultTab::adjustOctContrast()
{
	int min_dB = m_pLineEdit_OctDbMin->text().toInt();
	int max_dB = m_pLineEdit_OctDbMax->text().toInt();
	int ctable_ind = m_pComboBox_OctColorTable->currentIndex();

	m_pConfig->octDbRange.min = min_dB;
	m_pConfig->octDbRange.max = max_dB;
	
	m_pConfig->octColorTable = ctable_ind;

	m_pImageView_RectImage->resetColormap(ColorTable::colortable(ctable_ind));
	m_pImageView_CircImage->resetColormap(ColorTable::colortable(ctable_ind));
	m_pImageView_OctProjection->resetColormap(ColorTable::colortable(ctable_ind));	
	m_pImageView_ColorbarOctProjection->resetColormap(ColorTable::colortable(ctable_ind));

	ColorTable temp_ctable;

	if (m_pImgObjRectImage) delete m_pImgObjRectImage;
	m_pImgObjRectImage = new ImageObject(m_pImageView_RectImage->getRender()->m_pImage->width(), m_pImageView_RectImage->getRender()->m_pImage->height(), temp_ctable.m_colorTableVector.at(ctable_ind));
	if (m_pImgObjCircImage) delete m_pImgObjCircImage;
	m_pImgObjCircImage = new ImageObject(m_pImageView_CircImage->getRender()->m_pImage->width(), m_pImageView_CircImage->getRender()->m_pImage->height(), temp_ctable.m_colorTableVector.at(ctable_ind));

	visualizeEnFaceMap(true);
	visualizeImage(m_pSlider_SelectFrame->value());
}



void QResultTab::startProcessing()
{	
	m_pSlider_SelectFrame->setValue(0);

	int id = m_pButtonGroup_DataSelection->checkedId();
	switch (id)
	{
	case IN_BUFFER_DATA:
		inBufferDataProcessing();
		break;
	case EXTERNAL_DATA:
		externalDataProcessing();
		break;
	}
}




void QResultTab::inBufferDataProcessing()
{
	std::thread t1([&]() {

		std::chrono::system_clock::time_point start = std::chrono::system_clock::now();

		// Load configuration data ///////////////////////////////////////////////////////////////////
		Configuration *pConfig = m_pMainWnd->m_pConfiguration;
        if (m_pCheckBox_SingleFrame->isChecked()) pConfig->nFrames = 1;
		printf("Start in-buffer image processing... (Total nFrame: %d)\n", pConfig->nFrames);

		// Set Widgets //////////////////////////////////////////////////////////////////////////////
		emit setWidgets(false, pConfig);
        m_pImageView_RectImage->setUpdatesEnabled(false);
        m_pImageView_CircImage->setUpdatesEnabled(false);

        // Set Buffers //////////////////////////////////////////////////////////////////////////////
		setObjects(pConfig);

        //int bufferSize = (false == m_pCheckBox_SingleFrame->isChecked()) ? PROCESSING_BUFFER_SIZE : 1;
        //m_syncOctProcessing.allocate_queue_buffer(pConfig->nScans, pConfig->nAlines, bufferSize);

		// Set OCT Object ///////////////////////////////////////////////////////////////////////////
		OCTProcess* pOCT = m_pMainWnd->m_pStreamTab->m_pOCT;
			
		// OCT Process //////////////////////////////////////////////////////////////////////////////
		std::thread oct_proc([&]() { octProcessing(pOCT, pConfig, true); });

		// Wait for threads end /////////////////////////////////////////////////////////////////////
		oct_proc.join();

		// Generate en face maps ////////////////////////////////////////////////////////////////////
		getOctProjection(m_vectorOctImage, m_octProjection, m_pConfig->circShift);

		// Delete threading sync buffers ////////////////////////////////////////////////////////////
		//m_syncOctProcessing.deallocate_queue_buffer();

		// Reset Widgets /////////////////////////////////////////////////////////////////////////////
		emit setWidgets(true, pConfig);

		// Visualization /////////////////////////////////////////////////////////////////////////////
		visualizeEnFaceMap(true);	
		visualizeImage(0);

		std::chrono::duration<double> elapsed = std::chrono::system_clock::now() - start;
		printf("elapsed time : %.2f sec\n\n", elapsed.count());
	});

	t1.detach();
}

void QResultTab::externalDataProcessing()
{	
	// Get path to read
	QString fileName = QFileDialog::getOpenFileName(nullptr, "Load external OCT data", "", "OCT raw data (*.data)");
	if (fileName != "")
	{
		std::thread t1([&, fileName]() {

			std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
			
			QFile file(fileName);
			if (false == file.open(QFile::ReadOnly))
				printf("[ERROR] Invalid external data!\n");
			else
			{
				// Read Ini File & Initialization ///////////////////////////////////////////////////////////
				QString fileTitle;
				for (int i = 0; i < fileName.length(); i++)
					if (fileName.at(i) == QChar('.')) fileTitle = fileName.left(i);
				for (int i = 0; i < fileName.length(); i++)
					if (fileName.at(i) == QChar('/')) m_path = fileName.left(i);

				QString iniName = fileTitle + ".ini";
				QString bgName = fileTitle + ".background";
				QString calibName = fileTitle + ".calibration";

				qDebug() << iniName;

				static Configuration config;
				config.getConfigFile(iniName);
				if (m_pCheckBox_UserDefinedAlines->isChecked())
				{
					config.nAlines = m_pLineEdit_UserDefinedAlines->text().toInt();
					config.nAlines4 = ((config.nAlines + 3) >> 2) << 2;
					config.nFrameSize = config.nScans * config.nAlines;
				}
				config.octDiscomVal = m_pLineEdit_DiscomValue->text().toInt();
				config.nFrames = (int)(file.size() / (qint64)config.nScans / (qint64)config.nAlines / sizeof(uint16_t));
				if (m_pCheckBox_SingleFrame->isChecked()) config.nFrames = 1;

				printf("Start external image processing... (Total nFrame: %d)\n", config.nFrames);

				// Set Widgets //////////////////////////////////////////////////////////////////////////////
				emit setWidgets(false, &config);
				m_pImageView_RectImage->setUpdatesEnabled(false);
				m_pImageView_CircImage->setUpdatesEnabled(false);

				// Set Buffers //////////////////////////////////////////////////////////////////////////////
				setObjects(&config);

				int bufferSize = (false == m_pCheckBox_SingleFrame->isChecked()) ? PROCESSING_BUFFER_SIZE : 1;
				m_syncOctProcessing.allocate_queue_buffer(config.nScans, config.nAlines, bufferSize);

				// Set OCT Object ///////////////////////////////////////////////////////////////////////////
				OCTProcess* pOCT = new OCTProcess(config.nScans, config.nAlines);
				pOCT->loadCalibration(calibName.toUtf8().constData(), bgName.toUtf8().constData());
				pOCT->changeDiscomValue(config.octDiscomVal);
			
				// Get external data ////////////////////////////////////////////////////////////////////////
				std::thread load_data([&]() { loadingRawData(&file, &config); });

				// OCT Process //////////////////////////////////////////////////////////////////////////////
				std::thread oct_proc([&]() { octProcessing(pOCT, &config); });

				// Wait for threads end /////////////////////////////////////////////////////////////////////
				load_data.join();
				oct_proc.join();

				// Generate en face maps ////////////////////////////////////////////////////////////////////
				getOctProjection(m_vectorOctImage, m_octProjection, m_pConfig->circShift);

				// Delete OCT FLIM Object & threading sync buffers //////////////////////////////////////////
				delete pOCT; 
				m_syncOctProcessing.deallocate_queue_buffer();

				// Reset Widgets /////////////////////////////////////////////////////////////////////////////
				emit setWidgets(true, &config);
		
				// Visualization /////////////////////////////////////////////////////////////////////////////
                visualizeEnFaceMap(true);
				visualizeImage(0);
			}

			std::chrono::duration<double> elapsed = std::chrono::system_clock::now() - start;
			printf("elapsed time : %.2f sec\n\n", elapsed.count());
		});

		t1.detach();
	}
}

void QResultTab::setWidgetsEnabled(bool enabled, Configuration* pConfig)
{
	if (enabled)
	{
		// Reset visualization widgets
		m_pImageView_RectImage->setEnabled(true);
        m_pImageView_RectImage->setUpdatesEnabled(true);
		m_pImageView_CircImage->setEnabled(true);
        m_pImageView_CircImage->setUpdatesEnabled(true);

		m_pImageView_RectImage->resetSize(pConfig->nAlines4, pConfig->n2ScansFFT);
		m_pImageView_CircImage->resetSize(2 * CIRC_RADIUS, 2 * CIRC_RADIUS);

		m_pImageView_OctProjection->resetSize(pConfig->nAlines4, pConfig->nFrames);

		// Reset widgets
		m_pImageView_OctProjection->setEnabled(true);        
        m_pImageView_OctProjection->setUpdatesEnabled(true);

		m_pPushButton_StartProcessing->setEnabled(true);
        if (m_pRadioButton_External->isChecked() && (false == m_pCheckBox_SingleFrame->isChecked()))
            m_pPushButton_SaveResults->setEnabled(true);

		if ((m_pMemBuff->m_nRecordedFrames != 0) && (!m_pMemBuff->m_bIsSaved))
            m_pRadioButton_InBuffer->setEnabled(true);
		m_pRadioButton_External->setEnabled(true);

		m_pCheckBox_SingleFrame->setEnabled(true);
		m_pCheckBox_UserDefinedAlines->setEnabled(true);
		if (m_pCheckBox_UserDefinedAlines->isChecked())
			m_pLineEdit_UserDefinedAlines->setEnabled(true);
		m_pLabel_DiscomValue->setEnabled(true);
		m_pLineEdit_DiscomValue->setEnabled(true);

		m_pProgressBar_PostProcessing->setFormat("");
        m_pProgressBar_PostProcessing->setValue(0);

		if (m_pCheckBox_CircularizeImage->isChecked())
			m_pToggleButton_MeasureDistance->setEnabled(true);
		m_pLabel_SelectFrame->setEnabled(true);
		m_pSlider_SelectFrame->setEnabled(true);
		m_pSlider_SelectFrame->setRange(0, pConfig->nFrames - 1);
		m_pSlider_SelectFrame->setValue(0);
		
		m_pCheckBox_CircularizeImage->setEnabled(true);
		m_pCheckBox_ShowGuideLine->setEnabled(true);
		m_pLabel_CircShift->setEnabled(true);
		m_pLineEdit_CircShift->setEnabled(true);
		m_pLabel_OctColorTable->setEnabled(true);
		m_pComboBox_OctColorTable->setEnabled(true);

		m_pLineEdit_OctDbMax->setEnabled(true);
		m_pLineEdit_OctDbMin->setEnabled(true);

#ifdef GALVANO_MIRROR
		m_pDeviceControlTab->setScrollBarRange(pConfig->nAlines);
		m_pDeviceControlTab->setScrollBarEnabled(true);
		m_pDeviceControlTab->setScrollBarValue(pConfig->galvoHorizontalShift);
#endif
	}
	else
	{
		// Set widgets
        m_pImageView_RectImage->setDisabled(true);
        m_pImageView_RectImage->setUpdatesEnabled(false);
        m_pImageView_CircImage->setDisabled(true);
        m_pImageView_CircImage->setUpdatesEnabled(false);

		m_pImageView_OctProjection->setDisabled(true);
        m_pImageView_OctProjection->setUpdatesEnabled(false);

		m_pPushButton_StartProcessing->setDisabled(true);
		m_pPushButton_SaveResults->setDisabled(true);
		m_pRadioButton_InBuffer->setDisabled(true);
		m_pRadioButton_External->setDisabled(true);

		m_pCheckBox_SingleFrame->setDisabled(true);
		m_pCheckBox_UserDefinedAlines->setDisabled(true);
		m_pLineEdit_UserDefinedAlines->setDisabled(true);
		m_pLabel_DiscomValue->setDisabled(true);
		m_pLineEdit_DiscomValue->setDisabled(true);

		int id = m_pButtonGroup_DataSelection->checkedId();
		switch (id)
		{
		case IN_BUFFER_DATA:
			m_pProgressBar_PostProcessing->setFormat("In-buffer data processing... %p%");
			break;
		case EXTERNAL_DATA:
			m_pProgressBar_PostProcessing->setFormat("External data processing... %p%");
			break;
		}		
        m_pProgressBar_PostProcessing->setRange(0, (pConfig->nFrames != 1) ? pConfig->nFrames - 1 : 1);
		m_pProgressBar_PostProcessing->setValue(0);

		m_pToggleButton_MeasureDistance->setDisabled(true);
		m_pLabel_SelectFrame->setDisabled(true);
		QString str; str.sprintf("Current Frame : %3d / %3d", 1, pConfig->nFrames);
		m_pLabel_SelectFrame->setText(str);

		m_pSlider_SelectFrame->setDisabled(true);

		m_pCheckBox_CircularizeImage->setDisabled(true);
		m_pCheckBox_ShowGuideLine->setDisabled(true);
		m_pLabel_CircShift->setDisabled(true);
		m_pLineEdit_CircShift->setDisabled(true);
		m_pLabel_OctColorTable->setDisabled(true);
		m_pComboBox_OctColorTable->setDisabled(true);

		m_pLineEdit_OctDbMax->setDisabled(true);
		m_pLineEdit_OctDbMin->setDisabled(true);

#ifdef GALVANO_MIRROR
		m_pDeviceControlTab->setScrollBarEnabled(false);
		m_pDeviceControlTab->setScrollBarValue(0);
#endif
	}
}

void QResultTab::setWidgetsEnabled(bool enabled)
{
	if (enabled)
	{
		// Reset visualization widgets
		m_pImageView_RectImage->setEnabled(true);
		m_pImageView_CircImage->setEnabled(true);

		// Reset widgets
		m_pImageView_OctProjection->setEnabled(true);

		m_pPushButton_StartProcessing->setEnabled(true);
		m_pPushButton_SaveResults->setEnabled(true);
		if ((m_pMemBuff->m_nRecordedFrames != 0) && (!m_pMemBuff->m_bIsSaved))
			m_pRadioButton_InBuffer->setEnabled(true);
		m_pRadioButton_External->setEnabled(true);

		m_pCheckBox_SingleFrame->setEnabled(true);
		m_pCheckBox_UserDefinedAlines->setEnabled(true);
		if (m_pCheckBox_UserDefinedAlines->isChecked())
			m_pLineEdit_UserDefinedAlines->setEnabled(true);
		m_pLabel_DiscomValue->setEnabled(true);
		m_pLineEdit_DiscomValue->setEnabled(true);

		m_pProgressBar_PostProcessing->setFormat("");

		if (m_pCheckBox_CircularizeImage->isChecked())
			m_pToggleButton_MeasureDistance->setEnabled(true);
		m_pLabel_SelectFrame->setEnabled(true);
		m_pSlider_SelectFrame->setEnabled(true);

		m_pCheckBox_CircularizeImage->setEnabled(true);
		m_pCheckBox_ShowGuideLine->setEnabled(true);
		m_pLabel_CircShift->setEnabled(true);
		m_pLineEdit_CircShift->setEnabled(true);
		m_pLabel_OctColorTable->setEnabled(true);
		m_pComboBox_OctColorTable->setEnabled(true);

		m_pLineEdit_OctDbMax->setEnabled(true);
		m_pLineEdit_OctDbMin->setEnabled(true);

#ifdef GALVANO_MIRROR
		m_pDeviceControlTab->setScrollBarEnabled(true);
#endif
	}
	else
	{
		// Set widgets
        m_pImageView_RectImage->setDisabled(true);
        m_pImageView_CircImage->setDisabled(true);

		m_pImageView_OctProjection->setDisabled(true);

		m_pPushButton_StartProcessing->setDisabled(true);
		m_pPushButton_SaveResults->setDisabled(true);
		m_pRadioButton_InBuffer->setDisabled(true);
		m_pRadioButton_External->setDisabled(true);

		m_pCheckBox_SingleFrame->setDisabled(true);
		m_pCheckBox_UserDefinedAlines->setDisabled(true);
		m_pLineEdit_UserDefinedAlines->setDisabled(true);
		m_pLabel_DiscomValue->setDisabled(true);
		m_pLineEdit_DiscomValue->setDisabled(true);

		m_pProgressBar_PostProcessing->setFormat("Saving results... %p%");
		m_pProgressBar_PostProcessing->setRange(0, (int)m_vectorOctImage.size() * 2 - 1);
		m_pProgressBar_PostProcessing->setValue(0);

		m_pToggleButton_MeasureDistance->setDisabled(true);
		m_pLabel_SelectFrame->setDisabled(true);
		m_pSlider_SelectFrame->setDisabled(true);

		m_pCheckBox_CircularizeImage->setDisabled(true);
		m_pCheckBox_ShowGuideLine->setDisabled(true);
		m_pLabel_CircShift->setDisabled(true);
		m_pLineEdit_CircShift->setDisabled(true);
		m_pLabel_OctColorTable->setDisabled(true);
		m_pComboBox_OctColorTable->setDisabled(true);

		m_pLineEdit_OctDbMax->setDisabled(true);
		m_pLineEdit_OctDbMin->setDisabled(true);

#ifdef GALVANO_MIRROR
		m_pDeviceControlTab->setScrollBarEnabled(false);
#endif
	}
}

void QResultTab::setObjects(Configuration* pConfig)
{
	// Clear existed buffers
	std::vector<np::FloatArray2> clear_vector;
	clear_vector.swap(m_vectorOctImage);

	// Data buffers
	for (int i = 0; i < pConfig->nFrames; i++)
	{
		np::FloatArray2 buffer = np::FloatArray2(pConfig->n2ScansFFT, pConfig->nAlines4);
		m_vectorOctImage.push_back(buffer);
	}
	m_octProjection = np::FloatArray2(pConfig->nAlines4, pConfig->nFrames);

	// Visualization buffers
	ColorTable temp_ctable;

	if (m_pImgObjRectImage) delete m_pImgObjRectImage;
	m_pImgObjRectImage = new ImageObject(pConfig->nAlines4, pConfig->n2ScansFFT, temp_ctable.m_colorTableVector.at(m_pComboBox_OctColorTable->currentIndex()));
	if (m_pImgObjCircImage) delete m_pImgObjCircImage;
	m_pImgObjCircImage = new ImageObject(2 * CIRC_RADIUS, 2 * CIRC_RADIUS, temp_ctable.m_colorTableVector.at(m_pComboBox_OctColorTable->currentIndex()));

	// En face map visualization buffers
	m_visOctProjection = np::Uint8Array2(pConfig->nAlines4, pConfig->nFrames);

	// Circ & Medfilt objects
    if (m_pCirc) delete m_pCirc;
    m_pCirc = new circularize(CIRC_RADIUS, pConfig->nAlines, false);

	if (m_pMedfiltRect) delete m_pMedfiltRect;
    m_pMedfiltRect = new medfilt(pConfig->nAlines4, pConfig->n2ScansFFT, 3, 3);
}

void QResultTab::loadingRawData(QFile* pFile, Configuration* pConfig)
{
	int frameCount = 0;

	while (frameCount < pConfig->nFrames)
	{
		// Get buffers from threading queues
		uint16_t* frame_data = nullptr;
		do
		{
			{
				std::unique_lock<std::mutex> lock(m_syncOctProcessing.mtx);
				if (!m_syncOctProcessing.queue_buffer.empty())
				{
					frame_data = m_syncOctProcessing.queue_buffer.front();
					m_syncOctProcessing.queue_buffer.pop();
				}
			}

			if (frame_data)
			{
				// Read data from the external data 
				pFile->read(reinterpret_cast<char *>(frame_data), sizeof(uint16_t) * pConfig->nFrameSize);
				frameCount++;

				// Push the buffers to sync Queues
				m_syncOctProcessing.Queue_sync.push(frame_data);
			}
		} while (frame_data == nullptr);
	}
}

void QResultTab::octProcessing(OCTProcess* pOCT, Configuration* pConfig, bool inBuffer)
{
	MemoryBuffer* pMemBuff;
	if (inBuffer)
	{
		pMemBuff = m_pMainWnd->m_pOperationTab->m_pMemoryBuffer;
		pMemBuff->circulation(WRITING_BUFFER_SIZE - pConfig->nFrames);
	}

	int frameCount = 0;
	while (frameCount < pConfig->nFrames)
	{
		// Get the buffer from the previous sync Queue
		if (!inBuffer)
		{
			uint16_t* fringe_data = m_syncOctProcessing.Queue_sync.pop();

			if (fringe_data != nullptr)
			{
				// Body
				(*pOCT)(m_vectorOctImage.at(frameCount), fringe_data);
				emit processedSingleFrame(frameCount);

				frameCount++;

				// Return (push) the buffer to the previous threading queue
				{
					std::unique_lock<std::mutex> lock(m_syncOctProcessing.mtx);
					m_syncOctProcessing.queue_buffer.push(fringe_data);
				}
			}
			else
			{
				printf("octProcessing1 is halted.\n");
				break;
			}
		}
		else
		{
			// Pop front the buffer from the writing buffer queue
			uint16_t* fringe_data = pMemBuff->pop_front();

			// Body
			(*pOCT)(m_vectorOctImage.at(frameCount), fringe_data);
			emit processedSingleFrame(frameCount);

			frameCount++;

			// Push back the buffer to the writing buffer queue
			pMemBuff->push_back(fringe_data);
		}
	}
}


void QResultTab::getOctProjection(std::vector<np::FloatArray2>& vecImg, np::FloatArray2& octProj, int offset)
{
	int len = CIRC_RADIUS - (offset + PROJECTION_OFFSET);
	tbb::parallel_for(tbb::blocked_range<size_t>(0, (size_t)vecImg.size()),
		[&](const tbb::blocked_range<size_t>& r) {
		for (size_t i = r.begin(); i != r.end(); ++i)
		{
			float maxVal;
			for (int j = 0; j < octProj.size(0); j++)
			{
				ippsMax_32f(&vecImg.at((int)i)(offset + PROJECTION_OFFSET, j), len, &maxVal);
				octProj(j, (int)i) = maxVal;
			}
		}
	});
}
