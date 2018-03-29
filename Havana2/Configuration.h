#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#define VERSION						"1.0.0"

#define POWER_2(x)					(1 << x)
#define NEAR_2_POWER(x)				(int)(1 << (int)ceil(log2(x)))

///////////////////// Library enabling //////////////////////
#define NIIMAQ_ENABLE               true
#define NIDAQ_ENABLE				true

/////////////////////// System setup ////////////////////////
#if NIDAQ_ENABLE
//#define GALVANO_MIRROR
#endif
#define PULLBACK_DEVICE

//////////////////// Frame grabber setup ////////////////////
#if NIIMAQ_ENABLE
#define NUM_GRAB_BUFFS				10
#endif

/////////////////////// Device setup ////////////////////////
#ifdef GALVANO_MIRROR
#define NI_GALVO_CHANNEL			"Dev1/ao0:1"
#define NI_GAVLO_SOURCE				"/Dev1/PFI13"
#endif

#ifdef PULLBACK_DEVICE
#define ZABER_PORT					"COM4"
#define ZABER_MAX_MICRO_RESOLUTION  64
#define ZABER_MICRO_RESOLUTION		256
#define ZABER_CONVERSION_FACTOR		1.6384
#define ZABER_MICRO_STEPSIZE		0.09921875 // micro-meter

#define FAULHABER_PORT				"COM3"
#define FAULHABER_POSITIVE_ROTATION false
#endif

//////////////////////// Processing /////////////////////////
#define PROCESSING_BUFFER_SIZE		50
#define WIDTH_FILTER				51

#ifdef _DEBUG
#define WRITING_BUFFER_SIZE			50
#else
#define WRITING_BUFFER_SIZE	        1000
#endif

//////////////////////// OCT system /////////////////////////
#define DISCOM_VAL					0 

/////////////////////// Visualization ///////////////////////
#define CIRC_RADIUS					800 // It should be a multiple of 4.
#define PROJECTION_OFFSET			100

#define RENEWAL_COUNT				1






template <typename T>
struct Range
{
	T min = 0;
	T max = 0;
};

#include <QString>
#include <QSettings>
#include <QDateTime>

class Configuration
{
public:
	explicit Configuration() {}
	~Configuration() {}

public:
	void getConfigFile(QString inipath)
	{
		QSettings settings(inipath, QSettings::IniFormat);
		settings.beginGroup("configuration");

		// Digitizer setup
		offset = settings.value("offset").toInt();
		gain = settings.value("gain").toInt();
		lineTime = settings.value("lineTime").toInt();
		integTime = settings.value("integTime").toInt();

		acqLeft = settings.value("acqLeft").toInt();
		acqTop = settings.value("acqTop").toInt();
		acqWidth = settings.value("acqWidth").toInt();
		acqHeight = settings.value("acqHeight").toInt();

		nScans = acqWidth;
		nScansFFT = NEAR_2_POWER((double)nScans);
		n2ScansFFT = nScansFFT / 2;

		nAlines = acqHeight;
		nAlines4 = ((nAlines + 3) >> 2) << 2;

		nFrameSize = nScans * nAlines;

		// OCT processing
		octDiscomVal = settings.value("octDiscomVal").toInt();

		// Visualization
		circShift = settings.value("circShift").toInt();
		octColorTable = settings.value("octColorTable").toInt();
		octDbRange.max = settings.value("octDbRangeMax").toInt();
		octDbRange.min = settings.value("octDbRangeMin").toInt();

		// Device control
#ifdef GALVANO_MIRROR
		galvoFastScanVoltage = settings.value("galvoFastScanVoltage").toFloat();
		galvoFastScanVoltageOffset = settings.value("galvoFastScanVoltageOffset").toFloat();
		galvoSlowScanVoltage = settings.value("galvoSlowScanVoltage").toFloat();
		galvoSlowScanVoltageOffset = settings.value("galvoSlowScanVoltageOffset").toFloat();
		galvoSlowScanIncrement = settings.value("galvoSlowScanIncrement").toFloat();
		galvoHorizontalShift = 0;
#endif
#ifdef PULLBACK_DEVICE
		zaberPullbackSpeed = settings.value("zaberPullbackSpeed").toInt();
		zaberPullbackLength = settings.value("zaberPullbackLength").toInt();
		faulhaberRpm = settings.value("faulhaberRpm").toInt();
#endif

		settings.endGroup();
	}

	void setConfigFile(QString inipath)
	{
		QSettings settings(inipath, QSettings::IniFormat);
		settings.beginGroup("configuration");
				
		// Digitizer setup
		settings.setValue("offset", offset);
		settings.setValue("gain", gain);
		settings.setValue("lineTime", lineTime);
		settings.setValue("integTime", integTime);

		settings.setValue("acqLeft", acqLeft);
		settings.setValue("acqTop", acqTop);
		settings.setValue("acqWidth", acqWidth);
		settings.setValue("acqHeight", acqHeight);

		// OCT processing
		settings.setValue("octDiscomVal", octDiscomVal);

		// Visualization
		settings.setValue("circShift", circShift);
		settings.setValue("octColorTable", octColorTable);
		settings.setValue("octDbRangeMax", octDbRange.max);
		settings.setValue("octDbRangeMin", octDbRange.min);

		// Device control
#ifdef GALVANO_MIRROR
		settings.setValue("galvoFastScanVoltage", QString::number(galvoFastScanVoltage, 'f', 1));
		settings.setValue("galvoFastScanVoltageOffset", QString::number(galvoFastScanVoltageOffset, 'f', 1));
		settings.setValue("galvoSlowScanVoltage", QString::number(galvoSlowScanVoltage, 'f', 1));
		settings.setValue("galvoSlowScanVoltageOffset", QString::number(galvoSlowScanVoltageOffset, 'f', 1));
		settings.setValue("galvoSlowScanIncrement", QString::number(galvoSlowScanIncrement, 'f', 3));
		settings.setValue("galvoHorizontalShift", galvoHorizontalShift);
#endif
#ifdef PULLBACK_DEVICE
		settings.setValue("zaberPullbackSpeed", zaberPullbackSpeed);
		settings.setValue("zaberPullbackLength", zaberPullbackLength);
		settings.setValue("faulhaberRpm", faulhaberRpm);
#endif
		// Current Time
		QDate date = QDate::currentDate();
		QTime time = QTime::currentTime();
		settings.setValue("time", QString("%1-%2-%3 %4-%5-%6")
			.arg(date.year()).arg(date.month(), 2, 10, (QChar)'0').arg(date.day(), 2, 10, (QChar)'0')
			.arg(time.hour(), 2, 10, (QChar)'0').arg(time.minute(), 2, 10, (QChar)'0').arg(time.second(), 2, 10, (QChar)'0'));


		settings.endGroup();
	}
	
public:
	// Frame grabber & spectrometer setup
	int offset, gain;
	int lineTime, integTime;

	int acqLeft, acqTop;
	int acqWidth, acqHeight;

	// OCT size setup
	int nScans, nScansFFT, n2ScansFFT;
	int nAlines, nAlines4;
	int nFrames;

	int nFrameSize;
	
	// OCT processing
	int octDiscomVal;

	// Visualization
	int circShift;
	int octColorTable;
	Range<int> octDbRange;
	
	// Device control
#ifdef GALVANO_MIRROR
	float galvoFastScanVoltage;
	float galvoFastScanVoltageOffset;
	float galvoSlowScanVoltage;
	float galvoSlowScanVoltageOffset;
	float galvoSlowScanIncrement;
	int galvoHorizontalShift;
#endif
#ifdef PULLBACK_DEVICE
	int zaberPullbackSpeed;
	int zaberPullbackLength;
	int faulhaberRpm;
#endif
};

#endif // CONFIGURATION_H
