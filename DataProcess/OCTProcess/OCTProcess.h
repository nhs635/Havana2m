#ifndef _OCT_PROCESS_H_
#define _OCT_PROCESS_H_

#include <Havana2/Configuration.h>

#include <iostream>
#include <thread>
#include <complex>

#include <QString>
#include <QFile>

#include <ipps.h>
#include <ippvm.h>

#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>

#include <mkl_df.h>

#include <Common/array.h>
#include <Common/callback.h>
using namespace np;


struct FFT_R2C // 1D Fourier transformation for real signal (only for forward transformation)
{
public:
	FFT_R2C() :
        pFFTSpec(nullptr), pMemSpec(nullptr), pMemInit(nullptr), pMemBuffer(nullptr)
	{
	}

	~FFT_R2C()
	{
		if (pMemSpec) { ippsFree(pMemSpec); pMemSpec = nullptr; }
		if (pMemInit) { ippsFree(pMemInit); pMemInit = nullptr; }
		if (pMemBuffer) { ippsFree(pMemBuffer); pMemBuffer = nullptr; }
	}

	void operator() (Ipp32fc* dst, const Ipp32f* src, int line = 0)
	{
		ippsFFTFwd_RToPerm_32f(src, &temp(0, line), pFFTSpec, pMemBuffer);
		ippsConjPerm_32fc(&temp(0, line), dst, temp.size(0));
	}

	void initialize(int length, int line)
	{
		// init FFT spec
		const int ORDER = (int)(ceil(log2(length)));
		temp = FloatArray2(1 << ORDER, line); // for TBB operation to avoid thread risk

		int sizeSpec, sizeInit, sizeBuffer;
		ippsFFTGetSize_R_32f(ORDER, IPP_FFT_DIV_INV_BY_N, ippAlgHintNone, &sizeSpec, &sizeInit, &sizeBuffer);

		pMemSpec = ippsMalloc_8u(sizeSpec);
		pMemInit = ippsMalloc_8u(sizeInit);
		pMemBuffer = ippsMalloc_8u(sizeBuffer);

		ippsFFTInit_R_32f(&pFFTSpec, ORDER, IPP_FFT_DIV_INV_BY_N, ippAlgHintNone, pMemSpec, pMemInit);
	}

private:
	IppsFFTSpec_R_32f* pFFTSpec;
    Ipp8u* pMemSpec;
    Ipp8u* pMemInit;
    Ipp8u* pMemBuffer;

	FloatArray2 temp;
};

struct FFT_C2C // 1D Fourier transformation for complex signal (for both forward and inverse transformation)
{
	FFT_C2C() :
        pFFTSpec(nullptr), pMemSpec(nullptr), pMemInit(nullptr), pMemBuffer(nullptr)
	{
	}

	~FFT_C2C()
	{
		if (pMemSpec) { ippsFree(pMemSpec); pMemSpec = nullptr; }
		if (pMemInit) { ippsFree(pMemInit); pMemInit = nullptr; }
		if (pMemBuffer) { ippsFree(pMemBuffer); pMemBuffer = nullptr; }
	}

	void forward(Ipp32fc* dst, const Ipp32fc* src)
	{
		ippsFFTFwd_CToC_32fc(src, dst, pFFTSpec, pMemBuffer);
	}

	void inverse(Ipp32fc* dst, const Ipp32fc* src)
	{
		ippsFFTInv_CToC_32fc(src, dst, pFFTSpec, pMemBuffer);
	}

	void initialize(int length)
	{
		const int ORDER = (int)(ceil(log2(length)));

		int sizeSpec, sizeInit, sizeBuffer;
		ippsFFTGetSize_C_32fc(ORDER, IPP_FFT_DIV_INV_BY_N, ippAlgHintNone, &sizeSpec, &sizeInit, &sizeBuffer);

		pMemSpec = ippsMalloc_8u(sizeSpec);
		pMemInit = ippsMalloc_8u(sizeInit);
		pMemBuffer = ippsMalloc_8u(sizeBuffer);

		ippsFFTInit_C_32fc(&pFFTSpec, ORDER, IPP_FFT_DIV_INV_BY_N, ippAlgHintNone, pMemSpec, pMemInit);
	}

private:
	IppsFFTSpec_C_32fc* pFFTSpec;
    Ipp8u* pMemSpec;
    Ipp8u* pMemInit;
    Ipp8u* pMemBuffer;
};

struct MOVING_AVERAGE
{
	MOVING_AVERAGE() :
		pSpec(nullptr), pBuf(nullptr), pTaps(nullptr)
	{
	}

	~MOVING_AVERAGE()
	{
		if (pSpec) { ippsFree(pSpec); pSpec = nullptr; }
		if (pBuf) { ippsFree(pBuf); pBuf = nullptr; }
		if (pTaps) { ippsFree(pTaps); pTaps = nullptr; }
	}

	void operator() (Ipp32f* pDst, Ipp32f* pSrc)
	{
		ippsFIRSR_32f(pSrc, pDst, srcWidth, pSpec, NULL, NULL, pBuf);
	}

	void initialize(int _tapsLen, int _srcWidth)
	{
		tapsLen = _tapsLen;
		srcWidth = _srcWidth;

		pTaps = ippsMalloc_32f(tapsLen);
		ippsSet_32f(1.0f / (float)tapsLen, pTaps, tapsLen);

		ippsFIRSRGetSize(tapsLen, ipp32f, &specSize, &bufSize);
		pSpec = (IppsFIRSpec_32f*)ippsMalloc_8u(specSize);
		pBuf = ippsMalloc_8u(bufSize);
		ippsFIRSRInit_32f(pTaps, tapsLen, ippAlgDirect, pSpec);
	}

private:
	int tapsLen;
	int srcWidth;
	int specSize, bufSize;
	IppsFIRSpec_32f *pSpec;
	Ipp8u *pBuf;
	Ipp32f* pTaps;
};


class OCTProcess
{
// Methods
public: // Constructor & Destructor
	explicit OCTProcess(int nScans, int nAlines);
	~OCTProcess();
    
private: // Not to call copy constrcutor and copy assignment operator
    OCTProcess(const OCTProcess&);
    OCTProcess& operator=(const OCTProcess&);
	
public:
	// Generate OCT image
	void operator()(float* img, uint16_t* fringe);
	   
	// For calibration
    void setBg(const Uint16Array2& frame);
    void setFringe(const Uint16Array2& frame, int ch);

    float* getBg() { return bg.raw_ptr(); }
    float* getFringe(int ch) { return &fringe(0, ch); }

    void generateCalibration(int discom_val = 0);
    void changeDiscomValue(int discom_val = 0);
	void saveCalibration(QString calibpath = "calibration.dat");
	void loadCalibration(QString calibpath = "calibration.dat", QString bgpath = "bg.bin");

	// For calibration dialog
	callback2<float*, const char*> drawGraph;
	callback2<int&, int&> waitForRange;
	callback<void> endCalibration;
    
// Variables
private:
    // FFT objects
    FFT_R2C fft1; // fft
    FFT_C2C fft2; // ifft
    FFT_C2C fft3; // fft

	// Moving average objects
	MOVING_AVERAGE mov_avg;
    
    // Size variables
    IppiSize raw_size;
	IppiSize fft_size, fft2_size;
    
    // OCT image processing buffer
    FloatArray2 signal;
	FloatArray2 add_bg;
    ComplexFloatArray2 complex_signal;
    ComplexFloatArray2 complex_resamp;
    ComplexFloatArray2 fft_complex1;
    ComplexFloatArray2 fft_complex2;
    FloatArray2 fft_linear;
    
    // Calibration varialbes
    FloatArray bg;
    FloatArray2 fringe;
    FloatArray calib_index;
    FloatArray calib_weight;
    FloatArray win;
    ComplexFloatArray dispersion;
    ComplexFloatArray discom;
    ComplexFloatArray dispersion1;
};

#endif
