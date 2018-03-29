#ifndef DEVICESETUPDLG_H
#define DEVICESETUPDLG_H

#include <QObject>
#include <QtWidgets>
#include <QtCore>

#include <Havana2/Configuration.h>

#include <Common/array.h>

class MainWindow;
class QOperationTab;
class QStreamTab;
class QResultTab;
class DataAcquisition;
class MemoryBuffer;


class DeviceSetupDlg : public QDialog
{
    Q_OBJECT

// Constructer & Destructer /////////////////////////////
public:
    explicit DeviceSetupDlg(QWidget *parent = 0);
    virtual ~DeviceSetupDlg();
		
// Methods //////////////////////////////////////////////
private:
	void keyPressEvent(QKeyEvent *);

private slots:
	void changeOffset(const QString &);
	void changeGain(const QString &);
	void changeLineTime(const QString &);
	void changeIntegTime(const QString &);

	void changeAcqLeft(const QString &);
	void changeAcqTop(const QString &);
	void changeAcqWidth(const QString &);
	void changeAcqHeight(const QString &);

// Variables ////////////////////////////////////////////
private:
    MainWindow* m_pMainWnd;
	Configuration* m_pConfig;
	DataAcquisition* m_pDataAcq;
	MemoryBuffer* m_pMemBuff;
	QOperationTab* m_pOperationTab;
	QStreamTab* m_pStreamTab;
	QResultTab* m_pResultTab;

private:
	QLineEdit *m_pLineEdit_offset;
	QLineEdit *m_pLineEdit_gain;
	QLineEdit *m_pLineEdit_lineTime;
	QLineEdit *m_pLineEdit_integTime;

	QLineEdit *m_pLineEdit_acqLeft;
	QLineEdit *m_pLineEdit_acqTop;
	QLineEdit *m_pLineEdit_acqWidth;
	QLineEdit *m_pLineEdit_acqHeight;
};

#endif // DIGITIZERSETUPDLG_H
