#ifndef QSTREAMTAB_H
#define QSTREAMTAB_H

#include <QDialog>
#include <QtWidgets>
#include <QtCore>

#include <Havana2/Configuration.h>

#include <Common/array.h>
#include <Common/circularize.h>
#include <Common/medfilt.h>
#include <Common/SyncObject.h>
#include <Common/ImageObject.h>
#include <Common/basic_functions.h>

class MainWindow;
class QOperationTab;
class DataAcquisition;
class MemoryBuffer;

class QScope;
class QImageView;

class OCTProcess;
class ThreadManager;

class OctCalibDlg;


class QStreamTab : public QDialog
{
    Q_OBJECT

// Constructer & Destructer /////////////////////////////
public:
    explicit QStreamTab(QWidget *parent = 0);
	virtual ~QStreamTab();
	
// Methods //////////////////////////////////////////////
protected:
	void keyPressEvent(QKeyEvent *);

public:
	inline MainWindow* getMainWnd() const { return m_pMainWnd; }
	inline OctCalibDlg* getOctCalibDlg() const { return m_pOctCalibDlg; }

	inline int getCurrentAline() const { return m_pSlider_SelectAline->value(); }
	inline float getOctMaxDb() { return m_pLineEdit_OctDbMax->text().toFloat(); }
	inline float getOctMinDb() { return m_pLineEdit_OctDbMin->text().toFloat(); }
#ifdef GALVANO_MIRROR
	inline void invalidate() { visualizeImage(m_visImage.raw_ptr()); }
#endif
	void setWidgetsText();

private:
    void createOctVisualizationOptionTab();
		
private:
	// Set thread callback objects
	void setDataAcquisitionCallback();
	void setOctProcessingCallback();
	void setVisualizationCallback();

public: 
	void resetObjectsForAline(int nAlines);
	void visualizeImage(float* res); 

private slots:
	void updateAlinePos(int);
	void changeVisImage(bool);
	void checkCircShift(const QString &);
	void changeOctColorTable(int);
	void adjustOctContrast();	
	void createOctCalibDlg();
	void deleteOctCalibDlg();

signals:
	void plotFringe(float*);
	void plotAline(float*);
	void paintRectImage(uint8_t*);
	void paintCircImage(uint8_t*);


// Variables ////////////////////////////////////////////
private:
    MainWindow* m_pMainWnd;
	Configuration* m_pConfig;
	QOperationTab* m_pOperationTab;

	DataAcquisition* m_pDataAcq;
	MemoryBuffer* m_pMemBuff;

public:
	// Data process objects
	OCTProcess* m_pOCT;

public:
	// Thread manager objects
	ThreadManager* m_pThreadOctProcess;
	ThreadManager* m_pThreadVisualization;

private:
	// Thread synchronization objects
	SyncObject<uint16_t> m_syncOctProcessing;
	SyncObject<float> m_syncVisualization;

public:
	// Visualization buffers
	np::FloatArray2 m_visFringe;
	np::FloatArray2 m_visImage;
	
	ImageObject *m_pImgObjRectImage;
	ImageObject *m_pImgObjCircImage;

	circularize* m_pCirc;
	medfilt* m_pMedfilt;

private:
    // Layout
    QHBoxLayout *m_pHBoxLayout;

    // Graph viewer widgets
	QScope *m_pScope_OctFringe;
	QScope *m_pScope_OctDepthProfile;

    QSlider *m_pSlider_SelectAline;
    QLabel *m_pLabel_SelectAline;

    // Image viewer widgets
    QImageView *m_pImageView_RectImage;
    QImageView *m_pImageView_CircImage;

    // OCT visualization option widgets
	QGroupBox *m_pGroupBox_OctVisualization;

	QPushButton *m_pPushButton_OctCalibration;
    OctCalibDlg *m_pOctCalibDlg;

    QCheckBox *m_pCheckBox_CircularizeImage;   

    QLabel *m_pLabel_CircShift;
    QLineEdit *m_pLineEdit_CircShift;

    QLabel *m_pLabel_OctColorTable;
    QComboBox *m_pComboBox_OctColorTable;

    QLabel *m_pLabel_OctDb;
    QLineEdit *m_pLineEdit_OctDbMax;
    QLineEdit *m_pLineEdit_OctDbMin;
    QImageView *m_pImageView_OctDbColorbar;
};

#endif // QSTREAMTAB_H
