
#include "OCTProcess.h"
#include <Common/basic_functions.h>


OCTProcess::OCTProcess(int nScans, int nAlines) :
    
	raw_size({ nScans, nAlines }),
	fft_size({ (int)exp2(ceil(log2((double)nScans))), nAlines }),
	fft2_size({ fft_size.width / 2, nAlines}),
	
	signal(fft_size.width, fft_size.height),
	add_bg(fft_size.width, fft_size.height),
	complex_signal(fft_size.width, fft_size.height),
    complex_resamp(fft_size.width, fft_size.height),
	fft_complex1(fft_size.width, fft_size.height),
    fft_complex2(fft_size.width, fft_size.height),
	fft_linear(fft2_size.width, fft2_size.height), 	

	bg(raw_size.width),
    fringe(raw_size.width, 2),
	calib_index(raw_size.width),
    calib_weight(raw_size.width),
    win(raw_size.width),
    dispersion(raw_size.width),
    discom(raw_size.width),
    dispersion1(raw_size.width)
{   
	fft1.initialize(raw_size.width, raw_size.height);
	fft2.initialize(fft_size.width);
	fft3.initialize(raw_size.width);
	mov_avg.initialize(WIDTH_FILTER, raw_size.width);

	memset(bg.raw_ptr(), 0, sizeof(float) * bg.length());
    memset(fringe.raw_ptr(), 0, sizeof(float) * fringe.length());

    memset(signal.raw_ptr(), 0, sizeof(float) * signal.length());
    memset(complex_resamp.raw_ptr(), 0, sizeof(float) * 2 * complex_resamp.length());
    memset(fft_complex1.raw_ptr(), 0, sizeof(float) * 2 * fft_complex1.length());
	memset(fft_complex2.raw_ptr(), 0, sizeof(float) * 2 * fft_complex2.length());

	for (int i = 0; i < raw_size.width; i++)
	{
        bg(i)  = 0.0f;
		win(i) = (float)(1 - cos(IPP_2PI * i / (raw_size.width - 1))) / 2; // Hann Window
	}

	for (int i = 0; i < raw_size.width; i++)
	{
		calib_index(i) = (float)i;
		calib_weight(i) = 1;
		dispersion(i) = { 1, 0 };
		discom(i) = { 1, 0 };
		dispersion1(i) = { 1, 0 };
	}
}


OCTProcess::~OCTProcess()
{
}

#include <Havana2/Viewer/QScope.h>

/* OCT Image */
void OCTProcess::operator() (float* img, uint16_t* fringe)
{	
	tbb::parallel_for(tbb::blocked_range<size_t>(0, (size_t)raw_size.height),
		[&](const tbb::blocked_range<size_t>& r) {
		for (size_t i = r.begin(); i != r.end(); ++i)
		{
			int r1 = raw_size.width * (int)i;
			int f1 = fft_size.width * (int)i;
			int f2 = fft2_size.width * (int)i;

			// 1. Single Precision Conversion & Zero Padding
			ippsConvert_16u32f(fringe + r1, signal.raw_ptr() + f1, raw_size.width);
	
			// 2. BG Subtraction & Hanning Windowing
			ippsSub_32f_I(bg, signal.raw_ptr() + f1, raw_size.width);
			ippsMul_32f_I(win, signal.raw_ptr() + f1, raw_size.width);

			// 3. DC Background Removal
			mov_avg(add_bg.raw_ptr() + f1, signal.raw_ptr() + f1);
			std::rotate(add_bg.raw_ptr() + f1, add_bg.raw_ptr() + f1 + WIDTH_FILTER / 2, add_bg.raw_ptr() + f1 + raw_size.width);
			ippsSub_32f_I(add_bg.raw_ptr() + f1, signal.raw_ptr() + f1, raw_size.width);

			// 4. Fourier transform
			fft1((Ipp32fc*)(fft_complex1.raw_ptr() + f1), signal.raw_ptr() + f1, (int)i);

			// 5. Mirror Image Removal
			ippsSet_32f(0.0f, (Ipp32f*)(fft_complex1.raw_ptr() + f1 + fft_size.width / 2), fft_size.width);
			fft2.inverse((Ipp32fc*)(complex_signal.raw_ptr() + f1), (const Ipp32fc*)(fft_complex1.raw_ptr() + f1));
						
			// 6. k linear resampling	
            bf::LinearInterp_32fc((const Ipp32fc*)(complex_signal.raw_ptr() + f1), (Ipp32fc*)(complex_resamp.raw_ptr() + f1),
				raw_size.width, calib_index.raw_ptr(), calib_weight.raw_ptr());
			
			// 7. Dispersion compensation
			ippsMul_32fc_I((const Ipp32fc*)dispersion1.raw_ptr(), (Ipp32fc*)(complex_resamp.raw_ptr() + f1), raw_size.width);
			
			// 8. Fourier transform
			fft3.forward((Ipp32fc*)(fft_complex2.raw_ptr() + f1), (const Ipp32fc*)(complex_resamp.raw_ptr() + f1));

			// 9. dB Scaling
			ippsPowerSpectr_32fc((const Ipp32fc*)(fft_complex1.raw_ptr() + f1), fft_linear.raw_ptr() + f2, fft2_size.width);
			ippsLog10_32f_A11(fft_linear.raw_ptr() + f2, img + f2, fft2_size.width);
			ippsMulC_32f_I(10.0f, img + f2, fft2_size.width);
		}
	});
}


/* OCT Calibration */
void OCTProcess::setBg(const Uint16Array2& frame)
{
    int N = 50;

    for (int i = 0; i < frame.size(0); i++)
    {
        bg(i) = 0;
        for (int j = 0; j < N; j++)
            bg(i) += (float)frame(i, j);
        bg(i) /= N;
    }
}


void OCTProcess::setFringe(const Uint16Array2& frame, int ch)
{
    for (int i = 0; i < frame.size(0); i++)
        fringe(i, ch) = (float)frame(i, 0);
}

void OCTProcess::generateCalibration(int discom_val)
{
    std::thread calib([&, discom_val]() {

        // buffer
        np::Array<float, 2> fringe1(fft_size.width, 2);
        memset(fringe1.raw_ptr(), 0, sizeof(float) * fft_size.width * 2);
        np::Array<std::complex<float>, 2> res(fft_size.width, 2);
        np::Array<float, 2> res1(fft_size.width, 2);
        np::Array<float, 2> res2(fft_size.width, 2);
        np::Array<std::complex<float>, 2> res3(fft_size.width, 2);
        np::Array<float, 2> phase(raw_size.width, 2);

        for (int ch = 0; ch < 2; ch++)
        {
            // 0. BG removal & windowing
            for (int i = 0; i < raw_size.width; i++)
                fringe1(i, ch) = (fringe(i, ch) - bg(i)) * win(i);

            // 1. FFT of raw signal
            fft1((Ipp32fc*)&res(0, ch), &fringe1(0, ch));
            ippsPowerSpectr_32fc((const Ipp32fc*)&res(0, ch), (Ipp32f*)&res1(0, ch), fft_size.width);
            ippsLog10_32f_A11((const Ipp32f*)&res1(0, ch), (Ipp32f*)&res2(0, ch), fft_size.width);
            ippsMulC_32f_I(10.0f, (Ipp32f*)&res2(0, ch), fft_size.width);

            char title[50];
            sprintf(title, "FFT of d%d", ch + 1);
            drawGraph(&res2(0, ch), title);

            // 2. Maximum detection & Non-peak signal elimination
            int start1 = 0, end1 = 0;
			waitForRange(start1, end1);
			//printf("[%d %d]\n", start1, end1);

            np::Array<float> mask(fft_size.width);
            ippsZero_32f(mask, fft_size.width);
            ippsSet_32f(1.0f, &mask(start1), end1 - start1 + 1);

            ippsMul_32f32fc_I(mask.raw_ptr(), (Ipp32fc*)&res(0, ch), fft_size.width);

            // 3. Frequency shifting effect removal
            std::rotate(&res(0, ch), &res(3 * fft_size.width / 4, ch), &res(fft_size.width, ch));
            //ippsSet_32f(0.0f, (Ipp32f*)&res(fft_size.width / 4, ch), fft_size.width); // should be removed?

            // 4. IFFT of the signal & Phase extraction
            fft2.inverse((Ipp32fc*)&res3(0, ch), (Ipp32fc*)&res(0, ch));

            ippsPhase_32fc((const Ipp32fc*)&res3(0, ch), &phase(0, ch), raw_size.width);
            bf::UnwrapPhase_32f(&phase(0, ch), raw_size.width);
        }

        // 5. calib_index & calib_weight generation
        np::Array<float> index(raw_size.width); // 0 ~ nScans
        np::Array<float> lin_phase(raw_size.width);
        np::Array<float> new_index(raw_size.width);

        ippsSub_32f_I(&phase(0, 0), &phase(0, 1), raw_size.width); // phase(:,1) - phase(:,0)
        ippsAbs_32f_I(&phase(0, 1), raw_size.width); // absolute value

        bf::LineSpace_32f(0.0f, (float)(raw_size.width - 1), raw_size.width, index.raw_ptr());
        bf::LineSpace_32f(phase(0, 1), phase(raw_size.width - 1, 1), raw_size.width, lin_phase.raw_ptr());

		//FILE* pFile;
		//fopen_s(&pFile, "test.dat", "wb");
		//fwrite(&phase(0, 1), sizeof(float), raw_size.width, pFile);
		//fwrite(index.raw_ptr(), sizeof(float), raw_size.width, pFile);
		//fwrite(lin_phase.raw_ptr(), sizeof(float), raw_size.width, pFile);
		//fclose(pFile);

        bf::Interpolation_32f(&phase(0, 1), index.raw_ptr(), lin_phase.raw_ptr(), raw_size.width, raw_size.width, new_index.raw_ptr());

        float temp;
        for (int i = 0; i < raw_size.width; i++)
        {
            temp = floor(new_index(i));
            calib_weight(i) = 1.0f - (new_index(i) - temp);
            calib_index(i) = temp;
        }

        // 6. Dispersion compensation
        np::Array<float> index1(raw_size.width); // 0 ~ nScans/2
        np::Array<float> lin_phase1(raw_size.width);
        np::Array<float> lin_phase2(raw_size.width);
        bf::LineSpace_32f(0.0f, (float)(raw_size.width - 1), raw_size.width, index1.raw_ptr());

        Ipp32f offset, slope;
        bf::Interpolation_32f(index.raw_ptr(), &phase(0, 0), new_index.raw_ptr(), raw_size.width, raw_size.width, lin_phase1.raw_ptr()); // phase with disp : lin_phase1
        bf::LinearRegression_32f(index1.raw_ptr(), lin_phase1.raw_ptr(), raw_size.width, offset, slope); // poly fit
        ippsVectorSlope_32f(lin_phase2.raw_ptr(), raw_size.width, offset, slope); // poly val (phase without disp : lin_phase2)

        float* temp_disp = new float[raw_size.width];
        float* temp_filt  = new float[raw_size.width];
		for (int i = 0; i < raw_size.width; i++)
			temp_disp[i] = lin_phase2(i) - lin_phase1(i);     

		//bf::movingAverage_32f(temp_disp, temp_filt, raw_size.width, 15);
        //bf::polynomialFitting_32f(temp_disp, temp_fit, raw2_size.width, 4); // 4th polynomial fitting

        for (int i = 0; i < raw_size.width; i++)
            dispersion(i) = { cosf(temp_filt[i]), sinf(temp_filt[i]) };
		
        changeDiscomValue(discom_val);

        delete[] temp_disp;
        delete[] temp_filt;

        saveCalibration();
		endCalibration();

        printf("Successfully calibrated!\n");
    });

    calib.detach();
}


void OCTProcess::changeDiscomValue(int discom_val)
{
	double temp;
	for (int i = 0; i < raw_size.width; i++)
	{
		temp = (((double)i - (double)raw_size.width / 2.0f) / (double)raw_size.width); // *((i - raw2_size.width / 2) / raw2_size.width);
		discom(i) = { (float)cos((double)discom_val*temp*temp), (float)sin((double)discom_val*temp*temp) };
	}
	ippsMul_32fc((Ipp32fc*)dispersion.raw_ptr(), (Ipp32fc*)discom.raw_ptr(), (Ipp32fc*)dispersion1.raw_ptr(), raw_size.width);
}


void OCTProcess::saveCalibration(QString calibpath)
{
	qint64 sizeWrote, sizeTotalWrote = 0;
	
	// create file (calibration)
	QFile calibFile(calibpath);
	if (false != calibFile.open(QFile::WriteOnly))
	{
		// calib_index
		sizeWrote = calibFile.write(reinterpret_cast<char*>(calib_index.raw_ptr()), sizeof(float) * raw_size.width);
		sizeTotalWrote += sizeWrote;

		// calib_weight
		sizeWrote = calibFile.write(reinterpret_cast<char*>(calib_weight.raw_ptr()), sizeof(float) * raw_size.width);
		sizeTotalWrote += sizeWrote;

		// dispersion compensation real
		Ipp32f* real = ippsMalloc_32f(raw_size.width);
		ippsReal_32fc((const Ipp32fc*)dispersion.raw_ptr(), real, raw_size.width);
		sizeWrote = calibFile.write(reinterpret_cast<char*>(real), sizeof(float) * raw_size.width);
		ippsFree(real);
		sizeTotalWrote += sizeWrote;

		// dispersion compensation imag
		Ipp32f* imag = ippsMalloc_32f(raw_size.width);
		ippsImag_32fc((const Ipp32fc*)dispersion.raw_ptr(), imag, raw_size.width);
		sizeWrote = calibFile.write(reinterpret_cast<char*>(imag), sizeof(float) * raw_size.width);
		ippsFree(imag);
		sizeTotalWrote += sizeWrote;

		calibFile.close();
	}
	else
		printf("Calibration data cannot be saved.\n");
}


void OCTProcess::loadCalibration(QString calibpath, QString bgpath)
{
	qint64 sizeRead, sizeTotalRead = 0;

	printf("\n//// Load OCT Calibration Data ////\n");

	// create file (background)
	QFile bgFile(bgpath);

	Uint16Array2 frame(raw_size.width, raw_size.height);	

	if (false != bgFile.open(QIODevice::ReadOnly))
	{
		// background
		sizeRead = bgFile.read(reinterpret_cast<char*>(frame.raw_ptr()), sizeof(uint16_t) * frame.length());
		printf("Background data is successfully loaded.[%zu]\n", sizeRead);

		if (sizeRead)
		{
			if (frame.size(1) > 50)
			{
				int N = 50;
				for (int i = 0; i < frame.size(0); i++)
				{
					bg(i) = 0;
					for (int j = 0; j < N; j++)
						bg(i) += (float)frame(i, j);
					bg(i) /= N;
				}
			}
			else
			{
				printf("nAlines should be larger than 50.\n");
				return;
			}
		}

		bgFile.close();
	}
	else
		printf("Background data cannot be loaded.\n");

	// create file (calibration)
	QFile calibFile(calibpath);
	if (false != calibFile.open(QFile::ReadOnly))
	{
		// calib_index
		sizeRead = calibFile.read(reinterpret_cast<char*>(calib_index.raw_ptr()), sizeof(float) * raw_size.width);
		sizeTotalRead += sizeRead;
		printf("Calibration index is successfully loaded.[%zu]\n", sizeRead);

		// calib_weight
		sizeRead = calibFile.read(reinterpret_cast<char*>(calib_weight.raw_ptr()), sizeof(float) * raw_size.width);
		sizeTotalRead += sizeRead;
		printf("Calibration weight is successfully loaded.[%zu]\n", sizeRead);

		// dispersion compensation real
		Ipp32f* real = ippsMalloc_32f(raw_size.width);
		sizeRead = calibFile.read(reinterpret_cast<char*>(real), sizeof(float) * raw_size.width);
		sizeTotalRead += sizeRead;
		printf("Dispersion data (real) is successfully loaded.[%zu]\n", sizeRead);

		// dispersion compensation imag
		Ipp32f* imag = ippsMalloc_32f(raw_size.width);
		sizeRead = calibFile.read(reinterpret_cast<char*>(imag), sizeof(float) * raw_size.width);
		sizeTotalRead += sizeRead;
		printf("Dispersion data (imag) is successfully loaded.[%zu]\n", sizeRead);

		// dispersion compensation
		ippsRealToCplx_32f(real, imag, (Ipp32fc*)dispersion.raw_ptr(), raw_size.width);
		ippsFree(real); ippsFree(imag);
		changeDiscomValue(0);

		calibFile.close();
	}
	else
		printf("Calibration data cannot be loaded.\n");

	printf("\n");
}
