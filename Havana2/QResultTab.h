#ifndef QRESULTTAB_H
#define QRESULTTAB_H

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
#ifdef GALVANO_MIRROR
class QDeviceControlTab;
#endif
class MemoryBuffer;

class QImageView;
class SaveResultDlg;

class OCTProcess;


class QResultTab : public QDialog
{
    Q_OBJECT

// Constructer & Destructer /////////////////////////////
public:
    explicit QResultTab(QWidget *parent = 0);
	virtual ~QResultTab();

// Methods //////////////////////////////////////////////
protected:
	void keyPressEvent(QKeyEvent *);

public:
	inline MainWindow* getMainWnd() const { return m_pMainWnd; }
	inline SaveResultDlg* getSaveResultDlg() const { return m_pSaveResultDlg; }
	inline QRadioButton* getRadioInBuffer() const { return m_pRadioButton_InBuffer; }
	inline QProgressBar* getProgressBar() const { return m_pProgressBar_PostProcessing; }
	inline QImageView* getRectImageView() const { return m_pImageView_RectImage; }
	inline QImageView* getCircImageView() const { return m_pImageView_CircImage; }
	inline int getCurrentFrame() const { return m_pSlider_SelectFrame->value(); }
	inline int getCurrentOctColorTable() const { return m_pComboBox_OctColorTable->currentIndex(); }
	inline void setUserDefinedAlines(int nAlines) { m_pLineEdit_UserDefinedAlines->setText(QString::number(nAlines)); }
#ifdef GALVANO_MIRROR
	inline void invalidate() { visualizeEnFaceMap(true); visualizeImage(getCurrentFrame()); }
#endif
	void setWidgetsText();

private:
    void createDataLoadingWritingTab();
    void createVisualizationOptionTab();
    void createEnFaceMapTab();
	
private slots: // widget operation
	void changeDataSelection(int id);
	void createSaveResultDlg();
	void deleteSaveResultDlg();
	void enableUserDefinedAlines(bool);
	void visualizeImage(int);
	void visualizeEnFaceMap(bool scaling = true);
	void measureDistance(bool);
	void showGuideLine(bool);
	void changeVisImage(bool);
	void checkCircShift(const QString &);
	void adjustOctContrast();

private slots: // processing
	void startProcessing();
	void setWidgetsEnabled(bool, Configuration*);

public slots: // for saving
	void setWidgetsEnabled(bool);
	
signals:
	void setWidgets(bool, Configuration*);
	void setWidgets(bool);

	void paintRectImage(uint8_t*);
	void paintCircImage(uint8_t*);

	void paintOctProjection(uint8_t*);

	void processedSingleFrame(int);

private:
	void inBufferDataProcessing();
	void externalDataProcessing();

	void setObjects(Configuration* pConfig);

	void loadingRawData(QFile* pFile, Configuration* pConfig);
	void octProcessing(OCTProcess* pOCT, Configuration* pConfig, bool inBuffer = false);

private:
	void getOctProjection(std::vector<np::FloatArray2>& vecImg, np::FloatArray2& octProj, int offset);

// Variables ////////////////////////////////////////////
private: // main pointer
	MainWindow* m_pMainWnd;
	Configuration* m_pConfig;
#ifdef GALVANO_MIRROR
	QDeviceControlTab* m_pDeviceControlTab;
#endif
	MemoryBuffer* m_pMemBuff;

private: // for threading operation
	SyncObject<uint16_t> m_syncOctProcessing;

public: // for visualization
	std::vector<np::FloatArray2> m_vectorOctImage;
	np::FloatArray2 m_octProjection;

private:
	ImageObject *m_pImgObjRectImage;
	ImageObject *m_pImgObjCircImage;

	np::Uint8Array2 m_visOctProjection;

public:
	circularize* m_pCirc;
	medfilt* m_pMedfiltRect;


public:
	QString m_path;


private:
    // Layout
    QHBoxLayout *m_pHBoxLayout;

    // Image viewer widgets
    QImageView *m_pImageView_RectImage;
    QImageView *m_pImageView_CircImage;

    // Data loading & writing widgets
	QGroupBox *m_pGroupBox_DataLoadingWriting;

	QButtonGroup *m_pButtonGroup_DataSelection;
    QRadioButton *m_pRadioButton_InBuffer;
    QRadioButton *m_pRadioButton_External;

    QPushButton *m_pPushButton_StartProcessing;
    QPushButton *m_pPushButton_SaveResults;
	SaveResultDlg *m_pSaveResultDlg;

	QCheckBox *m_pCheckBox_UserDefinedAlines;
	QLineEdit *m_pLineEdit_UserDefinedAlines;

	QCheckBox *m_pCheckBox_SingleFrame;

	QLabel *m_pLabel_DiscomValue;
	QLineEdit *m_pLineEdit_DiscomValue;

	QProgressBar *m_pProgressBar_PostProcessing;

    // Visualization option widgets
	QGroupBox *m_pGroupBox_Visualization;

    QSlider *m_pSlider_SelectFrame;
    QLabel *m_pLabel_SelectFrame;

    QPushButton* m_pToggleButton_MeasureDistance;

    QCheckBox *m_pCheckBox_ShowGuideLine;
    QCheckBox *m_pCheckBox_CircularizeImage;

    QLabel *m_pLabel_CircShift;
    QLineEdit *m_pLineEdit_CircShift;

    QLabel *m_pLabel_OctColorTable;
    QComboBox *m_pComboBox_OctColorTable;

    // En face map tab widgets
	QGroupBox *m_pGroupBox_EnFace;
    QLabel *m_pLabel_OctProjection;

    QImageView *m_pImageView_OctProjection;
    QImageView *m_pImageView_ColorbarOctProjection;

    QLineEdit *m_pLineEdit_OctDbMax;
    QLineEdit *m_pLineEdit_OctDbMin;
};

#endif // QRESULTTAB_H
