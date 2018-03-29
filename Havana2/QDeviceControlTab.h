#ifndef QDEVICECONTROLTAB_H
#define QDEVICECONTROLTAB_H

#include <QDialog>
#include <QtWidgets>
#include <QtCore>

#include <Havana2/Configuration.h>

class MainWindow;
class QOperationTab;
class QStreamTab;
class QResultTab;

#ifdef GALVANO_MIRROR
#if NIDAQ_ENABLE
class GalvoScan;
#endif
#endif
#ifdef PULLBACK_DEVICE
class ZaberStage;
class FaulhaberMotor;
#endif


class QDeviceControlTab : public QDialog
{
    Q_OBJECT

// Constructer & Destructer /////////////////////////////
public:
    explicit QDeviceControlTab(QWidget *parent = 0);
    virtual ~QDeviceControlTab();

// Methods //////////////////////////////////////////////
protected:
	void closeEvent(QCloseEvent*);

public: ////////////////////////////////////////////////////////////////////////////////////////////////
    inline QVBoxLayout* getLayout() const { return m_pVBoxLayout; }
#ifdef GALVANO_MIRROR
	inline QCheckBox* getEnableGalvanoMirrorControl() const { return m_pCheckBox_GalvanoMirrorControl; }
#endif

private: ////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef GALVANO_MIRROR
    void createGalvanoMirrorControl();
#endif
#ifdef PULLBACK_DEVICE
    void createZaberStageControl();
    void createFaulhaberMotorControl();
#endif

public: ////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef GALVANO_MIRROR
	int getScrollBarValue() { return m_pScrollBar_ScanAdjustment->value(); }
	void setScrollBarValue(int pos); 
	void setScrollBarRange(int alines); 
	void setScrollBarEnabled(bool enable); 
#endif
#ifdef PULLBACK_DEVICE
	// Zaber Stage Control
	bool isZaberStageEnabled() { return m_pCheckBox_ZaberStageControl->isChecked(); }
	void pullback() { moveAbsolute(); }
#endif

private slots: //////////////////////////////////////////////////////////////////////////////////////////
#ifdef GALVANO_MIRROR
	// Galvano Mirror	
	void enableGalvanoMirror(bool);
	void triggering(bool);
	void changeGalvoFastScanVoltage(const QString &);
	void changeGalvoFastScanVoltageOffset(const QString &);
	void changeGalvoSlowScanVoltage(const QString &);
	void changeGalvoSlowScanVoltageOffset(const QString &);
	void changeGalvoSlowScanIncrement(const QString &);
	void scanAdjusting(int);
#endif
#ifdef PULLBACK_DEVICE
	// Zaber Stage Control
	void enableZaberStageControl(bool);
	void moveAbsolute();
	void setTargetSpeed(const QString &);
	void changeZaberPullbackLength(const QString &);
	void home();
	void stop();

	// Faulhaber Motor Control
	void enableFaulhaberMotorControl(bool);
	void rotate(bool);
	void changeFaulhaberRpm(const QString &);
#endif

// Variables ////////////////////////////////////////////
private: ////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef GALVANO_MIRROR
#if NIDAQ_ENABLE
	GalvoScan* m_pGalvoScan;
#endif
#endif
#ifdef PULLBACK_DEVICE
	// Zaber Stage Control
	ZaberStage* m_pZaberStage;
	// Faulhaber Motor Control
	FaulhaberMotor* m_pFaulhaberMotor;
#endif
	
private: ////////////////////////////////////////////////////////////////////////////////////////////////
	MainWindow* m_pMainWnd;
	Configuration* m_pConfig;
	QOperationTab* m_pOperationTab;


    // Layout
    QVBoxLayout *m_pVBoxLayout;

#ifdef GALVANO_MIRROR
    // Widgets for galvano mirror control
    QCheckBox *m_pCheckBox_GalvanoMirrorControl;
	QPushButton *m_pToggleButton_ScanTriggering;
    QLineEdit *m_pLineEdit_FastPeakToPeakVoltage;
    QLineEdit *m_pLineEdit_FastOffsetVoltage;
	QLineEdit *m_pLineEdit_SlowPeakToPeakVoltage;
	QLineEdit *m_pLineEdit_SlowOffsetVoltage;
	QLineEdit *m_pLineEdit_SlowScanIncrement;
    QLabel *m_pLabel_FastScanVoltage;
    QLabel *m_pLabel_FastScanPlusMinus;
    QLabel *m_pLabel_FastGalvanoVoltage;
	QLabel *m_pLabel_SlowScanVoltage;
	QLabel *m_pLabel_SlowScanPlusMinus;
	QLabel *m_pLabel_SlowGalvanoVoltage;
	QLabel *m_pLabel_SlowScanIncrement;
	QLabel *m_pLabel_SlowScanIncrementVoltage;
	QScrollBar *m_pScrollBar_ScanAdjustment;
    QLabel *m_pLabel_ScanAdjustment;
#endif
#ifdef PULLBACK_DEVICE
    // Widgets for Zaber stage control
    QCheckBox *m_pCheckBox_ZaberStageControl;
    QPushButton *m_pPushButton_SetTargetSpeed;
    QPushButton *m_pPushButton_MoveAbsolute;
    QPushButton *m_pPushButton_Home;
    QPushButton *m_pPushButton_Stop;
    QLineEdit *m_pLineEdit_TargetSpeed;
    QLineEdit *m_pLineEdit_TravelLength;
    QLabel *m_pLabel_TargetSpeed;
    QLabel *m_pLabel_TravelLength;

    // Widgets for Faulhaber motor control
    QCheckBox *m_pCheckBox_FaulhaberMotorControl;
    QPushButton *m_pToggleButton_Rotate;
    QLineEdit *m_pLineEdit_RPM;
    QLabel *m_pLabel_RPM;
#endif
};

#endif // QDEVICECONTROLTAB_H
