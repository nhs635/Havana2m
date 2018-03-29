#ifndef SAVERESULTDLG_H
#define SAVERESULTDLG_H

#include <QObject>
#include <QtWidgets>
#include <QtCore>

#include <Havana2/Configuration.h>

#include <iostream>
#include <vector>

#include <Common/array.h>
#include <Common/circularize.h>
#include <Common/medfilt.h>
#include <Common/Queue.h>
#include <Common/ImageObject.h>
#include <Common/basic_functions.h>

class MainWindow;
class QResultTab;

using ImgObjVector = std::vector<ImageObject*>; 

struct CrossSectionCheckList
{
	bool bRect, bCirc;
	bool bRectResize, bCircResize;
	int nRectWidth, nRectHeight;
	int nCircDiameter;
};

struct EnFaceCheckList
{
	bool bRaw;
	bool bScaled;
	bool bOctProj;
};


class SaveResultDlg : public QDialog
{
    Q_OBJECT

// Constructer & Destructer /////////////////////////////
public:
    explicit SaveResultDlg(QWidget *parent = 0);
    virtual ~SaveResultDlg();
		
// Methods //////////////////////////////////////////////
private:
	void closeEvent(QCloseEvent *e);
	void keyPressEvent(QKeyEvent *);

private slots:
	void saveCrossSections();
	void saveEnFaceMaps();
	void enableRectResize(bool);
	void enableCircResize(bool);
	void setWidgetsEnabled(bool);

signals:
	void setWidgets(bool);
	void savedSingleFrame(int);

private:
	void scaling(std::vector<np::FloatArray2>& vectorOctImage);
	void converting(CrossSectionCheckList checkList);
	void rectWriting(CrossSectionCheckList checkList);
	void circularizing(CrossSectionCheckList checkList);
	void circWriting(CrossSectionCheckList checkList);

// Variables ////////////////////////////////////////////
private:
    MainWindow* m_pMainWnd;
	Configuration* m_pConfig;
	QResultTab* m_pResultTab;

private:
	int m_nSavedFrames;

private: // for threading operation
	Queue<ImgObjVector*> m_syncQueueRectWriting;
	Queue<ImgObjVector*> m_syncQueueCircularizing;
	Queue<ImgObjVector*> m_syncQueueCircWriting;

private:
	// Save Cross-sections
	QPushButton* m_pPushButton_SaveCrossSections;
	QCheckBox* m_pCheckBox_RectImage;
	QCheckBox* m_pCheckBox_ResizeRectImage;
	QLineEdit* m_pLineEdit_RectWidth;
	QLineEdit* m_pLineEdit_RectHeight;
	QCheckBox* m_pCheckBox_CircImage;
	QCheckBox* m_pCheckBox_ResizeCircImage;
	QLineEdit* m_pLineEdit_CircDiameter;

	// Save En Face Maps
	QPushButton* m_pPushButton_SaveEnFaceMaps;
	QCheckBox* m_pCheckBox_RawData;
	QCheckBox* m_pCheckBox_ScaledImage;
	QCheckBox* m_pCheckBox_OctMaxProjection;
};

#endif // SAVERESULTDLG_H
