
#include "SaveResultDlg.h"

#include <Havana2/MainWindow.h>
#include <Havana2/QResultTab.h>
#include <Havana2/Viewer/QImageView.h>

#include <ippi.h>
#include <ippcc.h>

#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>

#include <iostream>
#include <thread>
#include <chrono>
#include <utility>


SaveResultDlg::SaveResultDlg(QWidget *parent) :
    QDialog(parent)
{
    // Set default size & frame
	setFixedSize(420, 100);
    setWindowFlags(Qt::Tool);
	setWindowTitle("Save Result");

    // Set main window objects
    m_pResultTab = (QResultTab*)parent;
    m_pMainWnd = m_pResultTab->getMainWnd();
	m_pConfig = m_pMainWnd->m_pConfiguration;

    // Create widgets for saving results (save Ccoss-sections)
	m_pPushButton_SaveCrossSections = new QPushButton(this);
	m_pPushButton_SaveCrossSections->setText("Save Cross-Sections");
	
	m_pCheckBox_RectImage = new QCheckBox(this);
	m_pCheckBox_RectImage->setText("Rect Image");
	m_pCheckBox_RectImage->setChecked(true);
	m_pCheckBox_CircImage = new QCheckBox(this);
	m_pCheckBox_CircImage->setText("Circ Image");
	m_pCheckBox_CircImage->setChecked(true);

	m_pCheckBox_ResizeRectImage = new QCheckBox(this);
	m_pCheckBox_ResizeRectImage->setText("Resize (w x h)");
	m_pCheckBox_ResizeCircImage = new QCheckBox(this);
	m_pCheckBox_ResizeCircImage->setText("Resize (diameter)");

	m_pLineEdit_RectWidth = new QLineEdit(this);
	m_pLineEdit_RectWidth->setFixedWidth(35);
	m_pLineEdit_RectWidth->setText(QString::number(m_pResultTab->m_vectorOctImage.at(0).size(0)));
	m_pLineEdit_RectWidth->setAlignment(Qt::AlignCenter);
	m_pLineEdit_RectWidth->setDisabled(true);
	m_pLineEdit_RectHeight = new QLineEdit(this);
	m_pLineEdit_RectHeight->setFixedWidth(35);
	m_pLineEdit_RectHeight->setText(QString::number(m_pResultTab->m_vectorOctImage.at(0).size(1)));
	m_pLineEdit_RectHeight->setAlignment(Qt::AlignCenter);
	m_pLineEdit_RectHeight->setDisabled(true);
	m_pLineEdit_CircDiameter = new QLineEdit(this);
	m_pLineEdit_CircDiameter->setFixedWidth(35);
	m_pLineEdit_CircDiameter->setText(QString::number(2 * CIRC_RADIUS));
	m_pLineEdit_CircDiameter->setAlignment(Qt::AlignCenter);
	m_pLineEdit_CircDiameter->setDisabled(true);

	// Save En Face Maps
	m_pPushButton_SaveEnFaceMaps = new QPushButton(this);
	m_pPushButton_SaveEnFaceMaps->setText("Save En Face Maps");

	m_pCheckBox_RawData = new QCheckBox(this);
	m_pCheckBox_RawData->setText("Raw Data");
	m_pCheckBox_RawData->setChecked(false);
	m_pCheckBox_ScaledImage = new QCheckBox(this);
	m_pCheckBox_ScaledImage->setText("Scaled Image");
	m_pCheckBox_ScaledImage->setChecked(true);

	m_pCheckBox_OctMaxProjection = new QCheckBox(this);
	m_pCheckBox_OctMaxProjection->setText("OCT Max Projection");
	m_pCheckBox_OctMaxProjection->setChecked(true);

	// Set layout
	QGridLayout *pGridLayout = new QGridLayout;
	pGridLayout->setSpacing(3);

	pGridLayout->addWidget(m_pPushButton_SaveCrossSections, 0, 0);
	
	QHBoxLayout *pHBoxLayout_RectResize = new QHBoxLayout;
	pHBoxLayout_RectResize->addWidget(m_pCheckBox_ResizeRectImage);
	pHBoxLayout_RectResize->addWidget(m_pLineEdit_RectWidth);
	pHBoxLayout_RectResize->addWidget(m_pLineEdit_RectHeight);
	pHBoxLayout_RectResize->addStretch(1);
	//pHBoxLayout_RectResize->addItem(new QSpacerItem(0, 0, QSizePolicy::Preferred, QSizePolicy::Fixed));

	pGridLayout->addWidget(m_pCheckBox_RectImage, 0, 1);
	pGridLayout->addItem(pHBoxLayout_RectResize, 0, 2, 1, 2);

	QHBoxLayout *pHBoxLayout_CircResize = new QHBoxLayout;
	pHBoxLayout_CircResize->addWidget(m_pCheckBox_ResizeCircImage);
	pHBoxLayout_CircResize->addWidget(m_pLineEdit_CircDiameter);
	pHBoxLayout_CircResize->addStretch(1);
	//pHBoxLayout_CircResize->addItem(new QSpacerItem(0, 0, QSizePolicy::Preferred, QSizePolicy::Fixed));

	pGridLayout->addWidget(m_pCheckBox_CircImage, 1, 1);
	pGridLayout->addItem(pHBoxLayout_CircResize, 1, 2, 1, 2);

	pGridLayout->addWidget(m_pPushButton_SaveEnFaceMaps, 4, 0);

	pGridLayout->addWidget(m_pCheckBox_RawData, 4, 1);
	pGridLayout->addWidget(m_pCheckBox_ScaledImage, 4, 2);

	pGridLayout->addWidget(m_pCheckBox_OctMaxProjection, 6, 1, 1, 2);
	
    setLayout(pGridLayout);

    // Connect
	connect(m_pPushButton_SaveCrossSections, SIGNAL(clicked(bool)), this, SLOT(saveCrossSections()));
	connect(m_pPushButton_SaveEnFaceMaps, SIGNAL(clicked(bool)), this, SLOT(saveEnFaceMaps()));

	connect(m_pCheckBox_ResizeRectImage, SIGNAL(toggled(bool)), SLOT(enableRectResize(bool)));
	connect(m_pCheckBox_ResizeCircImage, SIGNAL(toggled(bool)), SLOT(enableCircResize(bool)));

	connect(m_pResultTab, SIGNAL(setWidgets(bool)), m_pResultTab, SLOT(setWidgetsEnabled(bool)));
	connect(this, SIGNAL(setWidgets(bool)), this, SLOT(setWidgetsEnabled(bool)));
	connect(this, SIGNAL(savedSingleFrame(int)), m_pResultTab->getProgressBar(), SLOT(setValue(int)));
}

SaveResultDlg::~SaveResultDlg()
{
}

void SaveResultDlg::closeEvent(QCloseEvent *e)
{
	if (!m_pPushButton_SaveCrossSections->isEnabled())
		e->ignore();
	else
		finished(0);
}

void SaveResultDlg::keyPressEvent(QKeyEvent *e)
{
	if (e->key() != Qt::Key_Escape)
		QDialog::keyPressEvent(e);
}


void SaveResultDlg::saveCrossSections()
{
	std::thread t1([&]() {
		
		// Check Status /////////////////////////////////////////////////////////////////////////////
		CrossSectionCheckList checkList;
		checkList.bRect = m_pCheckBox_RectImage->isChecked();
		checkList.bCirc = m_pCheckBox_CircImage->isChecked();
		checkList.bRectResize = m_pCheckBox_ResizeRectImage->isChecked();
		checkList.bCircResize = m_pCheckBox_ResizeCircImage->isChecked();
		checkList.nRectWidth = m_pLineEdit_RectWidth->text().toInt();
		checkList.nRectHeight = m_pLineEdit_RectHeight->text().toInt();
		checkList.nCircDiameter = m_pLineEdit_CircDiameter->text().toInt();
		
		// Set Widgets //////////////////////////////////////////////////////////////////////////////
		emit setWidgets(false);
		emit m_pResultTab->setWidgets(false);
		m_nSavedFrames = 0;

		// Scaling Images ///////////////////////////////////////////////////////////////////////////
		std::thread scaleImages([&]() { scaling(m_pResultTab->m_vectorOctImage); });

		// Rect Writing /////////////////////////////////////////////////////////////////////////////
		std::thread writeRectImages([&]() { rectWriting(checkList); });

		// Circularizing ////////////////////////////////////////////////////////////////////////////		
		std::thread circularizeImages([&]() { circularizing(checkList); });

		// Circ Writing /////////////////////////////////////////////////////////////////////////////
		std::thread writeCircImages([&]() { circWriting(checkList); });

		// Wait for threads end /////////////////////////////////////////////////////////////////////
		scaleImages.join();
		writeRectImages.join();
		circularizeImages.join();
		writeCircImages.join();		

		// Reset Widgets ////////////////////////////////////////////////////////////////////////////
		emit setWidgets(true);
		emit m_pResultTab->setWidgets(true);
	});

	t1.detach();
}

void SaveResultDlg::saveEnFaceMaps()
{
	std::thread t1([&]() {

		// Check Status /////////////////////////////////////////////////////////////////////////////
		EnFaceCheckList checkList;
		checkList.bRaw = m_pCheckBox_RawData->isChecked();
		checkList.bScaled = m_pCheckBox_ScaledImage->isChecked();
		checkList.bOctProj = m_pCheckBox_OctMaxProjection->isChecked();

		// Set Widgets //////////////////////////////////////////////////////////////////////////////
		emit setWidgets(false);

		// Create directory for saving en face maps /////////////////////////////////////////////////
		QString enFacePath = m_pResultTab->m_path + "/en_face/";
		if (checkList.bRaw || checkList.bScaled) QDir().mkdir(enFacePath);

		// Raw en face map writing //////////////////////////////////////////////////////////////////
		if (checkList.bRaw)
		{
			if (checkList.bOctProj)
			{
				QFile fileOctMaxProj(enFacePath + "oct_max_projection.enface");
				if (false != fileOctMaxProj.open(QIODevice::WriteOnly))
				{
					fileOctMaxProj.write(reinterpret_cast<char*>(m_pResultTab->m_octProjection.raw_ptr()), sizeof(float) * m_pResultTab->m_octProjection.length());
					fileOctMaxProj.close();
				}
			}
		}

		// Scaled en face map writing ///////////////////////////////////////////////////////////////
		if (checkList.bScaled)
		{
			ColorTable temp_ctable;

			if (checkList.bOctProj)
			{
				IppiSize roi_proj = { m_pResultTab->m_octProjection.size(0), m_pResultTab->m_octProjection.size(1) };
				ImageObject imgObjOctMaxProj(roi_proj.width, roi_proj.height, temp_ctable.m_colorTableVector.at(m_pResultTab->getCurrentOctColorTable()));

				ippiScale_32f8u_C1R(m_pResultTab->m_octProjection, sizeof(float) * roi_proj.width, imgObjOctMaxProj.arr.raw_ptr(), sizeof(uint8_t) * roi_proj.width, roi_proj, m_pConfig->octDbRange.min, m_pConfig->octDbRange.max);
				ippiMirror_8u_C1IR(imgObjOctMaxProj.arr.raw_ptr(), sizeof(uint8_t) * roi_proj.width, roi_proj, ippAxsHorizontal);
#ifdef GALVANO_MIRROR
				if (m_pConfig->galvoHorizontalShift)
				{
					for (int i = 0; i < roi_proj.height; i++)
					{
						uint8_t* pImg = imgObjOctMaxProj.arr.raw_ptr() + i * roi_proj.width;
						std::rotate(pImg, pImg + m_pConfig->galvoHorizontalShift, pImg + roi_proj.width);
					}
				}
#endif
				imgObjOctMaxProj.qindeximg.save(enFacePath + "oct_max_projection.bmp", "bmp");
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(500));

		// Scaling MATLAB Script ////////////////////////////////////////////////////////////////////
		if (false == QFile::copy("scale_indicator.m", enFacePath + "scale_indicator.m"))
			printf("Error occurred while copying matlab sciprt.\n");

		// Reset Widgets ////////////////////////////////////////////////////////////////////////////
		emit setWidgets(true);
	});

	t1.detach();
}

void SaveResultDlg::enableRectResize(bool checked)
{
	m_pLineEdit_RectWidth->setEnabled(checked);
	m_pLineEdit_RectHeight->setEnabled(checked);
}

void SaveResultDlg::enableCircResize(bool checked)
{
	m_pLineEdit_CircDiameter->setEnabled(checked);
}

void SaveResultDlg::setWidgetsEnabled(bool enabled)
{
	// Save Cross-sections
	m_pPushButton_SaveCrossSections->setEnabled(enabled);
	m_pCheckBox_RectImage->setEnabled(enabled);
	m_pCheckBox_CircImage->setEnabled(enabled);

	m_pCheckBox_ResizeRectImage->setEnabled(enabled);
	m_pCheckBox_ResizeCircImage->setEnabled(enabled);

	m_pLineEdit_RectWidth->setEnabled(enabled);
	m_pLineEdit_RectHeight->setEnabled(enabled);
	m_pLineEdit_CircDiameter->setEnabled(enabled);
	if (enabled)
	{
		if (!m_pCheckBox_ResizeRectImage->isChecked())
		{
			m_pLineEdit_RectWidth->setEnabled(false);
			m_pLineEdit_RectHeight->setEnabled(false);
		}
		if (!m_pCheckBox_ResizeCircImage->isChecked())
			m_pLineEdit_CircDiameter->setEnabled(false);
	}	

	// Save En Face Maps
	m_pPushButton_SaveEnFaceMaps->setEnabled(enabled);
	m_pCheckBox_RawData->setEnabled(enabled);
	m_pCheckBox_ScaledImage->setEnabled(enabled);
	m_pCheckBox_OctMaxProjection->setEnabled(enabled);
}


void SaveResultDlg::scaling(std::vector<np::FloatArray2>& vectorOctImage)
{
	int nTotalFrame = (int)vectorOctImage.size();
	ColorTable temp_ctable;
	
	int frameCount = 0;
	while (frameCount < nTotalFrame)
	{
		// Create Image Object Array for threading operation
		IppiSize roi_oct = { vectorOctImage.at(0).size(0), vectorOctImage.at(0).size(1) };

		ImgObjVector* pImgObjVec = new ImgObjVector;

		// Image objects for OCT Images
		pImgObjVec->push_back(new ImageObject(roi_oct.height, roi_oct.width, temp_ctable.m_colorTableVector.at(m_pResultTab->getCurrentOctColorTable())));

		// OCT Visualization
		np::Uint8Array2 scale_temp(roi_oct.width, roi_oct.height);
		ippiScale_32f8u_C1R(vectorOctImage.at(frameCount), roi_oct.width * sizeof(float),
			scale_temp.raw_ptr(), roi_oct.width * sizeof(uint8_t), roi_oct, m_pConfig->octDbRange.min, m_pConfig->octDbRange.max);
		ippiTranspose_8u_C1R(scale_temp.raw_ptr(), roi_oct.width * sizeof(uint8_t), pImgObjVec->at(0)->arr.raw_ptr(), roi_oct.height * sizeof(uint8_t), roi_oct);
#ifdef GALVANO_MIRROR
		if (m_pConfig->galvoHorizontalShift)
		{
			for (int i = 0; i < roi_oct.width; i++)
			{
				uint8_t* pImg = pImgObjVec->at(0)->arr.raw_ptr() + i * roi_oct.height;
				std::rotate(pImg, pImg + m_pConfig->galvoHorizontalShift, pImg + roi_oct.height);
			}
		}
#endif
		(*m_pResultTab->m_pMedfiltRect)(pImgObjVec->at(0)->arr.raw_ptr());

		frameCount++;

		// Push the buffers to sync Queues
		m_syncQueueRectWriting.push(pImgObjVec);
	}
}

void SaveResultDlg::rectWriting(CrossSectionCheckList checkList)
{
	int nTotalFrame = (int)m_pResultTab->m_vectorOctImage.size();
	QString folderName;
	for (int i = 0; i < m_pResultTab->m_path.length(); i++)
		if (m_pResultTab->m_path.at(i) == QChar('/')) folderName = m_pResultTab->m_path.right(m_pResultTab->m_path.length() - i - 1);

	QString rectPath;
	rectPath = m_pResultTab->m_path + "/rect_image/";
	if (checkList.bRect) QDir().mkdir(rectPath);

	int frameCount = 0;
	while (frameCount < nTotalFrame)
	{
		// Get the buffer from the previous sync Queue
		ImgObjVector *pImgObjVec = m_syncQueueRectWriting.pop();	

		// Write rect images
		if (checkList.bRect)
		{			
			if (!checkList.bRectResize)
				pImgObjVec->at(0)->qindeximg.save(rectPath + QString("rect_%1_%2.bmp").arg(folderName).arg(frameCount + 1, 3, 10, (QChar)'0'), "bmp");
			else
				pImgObjVec->at(0)->qindeximg.scaled(checkList.nRectWidth, checkList.nRectHeight).
					save(rectPath + QString("rect_%1_%2.bmp").arg(folderName).arg(frameCount + 1, 3, 10, (QChar)'0'), "bmp");
		}
		
		frameCount++;
		emit savedSingleFrame(m_nSavedFrames++);

		// Push the buffers to sync Queues
		m_syncQueueCircularizing.push(pImgObjVec);
	}
}

void SaveResultDlg::circularizing(CrossSectionCheckList checkList)
{
	int nTotalFrame = (int)m_pResultTab->m_vectorOctImage.size();
	ColorTable temp_ctable;

	int frameCount = 0;
	while (frameCount < nTotalFrame)
	{
		// Get the buffer from the previous sync Queue
		ImgObjVector *pImgObjVec = m_syncQueueCircularizing.pop();
		ImgObjVector *pImgObjVecCirc = new ImgObjVector;

		// ImageObject for circ writing
		ImageObject *pCircImgObj = new ImageObject(2 * CIRC_RADIUS, 2 * CIRC_RADIUS, temp_ctable.m_colorTableVector.at(m_pResultTab->getCurrentOctColorTable()));

		// Circularize
		if (checkList.bCirc)
		{
			np::Uint8Array2 rect_temp(pImgObjVec->at(0)->qindeximg.bits(), pImgObjVec->at(0)->arr.size(0), pImgObjVec->at(0)->arr.size(1));
			(*m_pResultTab->m_pCirc)(rect_temp, pCircImgObj->qindeximg.bits(), "vertical", m_pConfig->circShift);
		}

		// Vector pushing back
		pImgObjVecCirc->push_back(pCircImgObj);

		frameCount++;

		// Push the buffers to sync Queues
		m_syncQueueCircWriting.push(pImgObjVecCirc);

		// Delete ImageObjects
		delete pImgObjVec->at(0);
		delete pImgObjVec;
	}
}

void SaveResultDlg::circWriting(CrossSectionCheckList checkList)
{
	int nTotalFrame = (int)m_pResultTab->m_vectorOctImage.size();
	QString folderName;
	for (int i = 0; i < m_pResultTab->m_path.length(); i++)
		if (m_pResultTab->m_path.at(i) == QChar('/')) folderName = m_pResultTab->m_path.right(m_pResultTab->m_path.length() - i - 1);

	QString circPath;
	circPath = m_pResultTab->m_path + "/circ_image/";
	if (checkList.bCirc) QDir().mkdir(circPath);

	int frameCount = 0;
	while (frameCount < nTotalFrame)
	{
		// Get the buffer from the previous sync Queue
		ImgObjVector *pImgObjVecCirc = m_syncQueueCircWriting.pop();

		// Write circ images
		if (checkList.bCirc)
		{
			if (!checkList.bCircResize)
				pImgObjVecCirc->at(0)->qindeximg.save(circPath + QString("circ_%1_%2.bmp").arg(folderName).arg(frameCount + 1, 3, 10, (QChar)'0'), "bmp");
			else
				pImgObjVecCirc->at(0)->qindeximg.scaled(checkList.nCircDiameter, checkList.nCircDiameter).
					save(circPath + QString("circ_%1_%2.bmp").arg(folderName).arg(frameCount + 1, 3, 10, (QChar)'0'), "bmp");
		}
		
		frameCount++;
		emit savedSingleFrame(m_nSavedFrames++);

		// Delete ImageObjects
		delete pImgObjVecCirc->at(0);
		delete pImgObjVecCirc;
	}
}