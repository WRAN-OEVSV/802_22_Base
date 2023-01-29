#pragma once

#include "liquid.h"

#define PHY_SUBCARRIERS__M        1024
#define PHY_TAPERLEN              32

#define PHY_CP_STS_LTS            256
#define PHY_STS_DETECT_LWR_THR    2.0f
#define PHY_STS_DETECT_UPR_THR    3.0f



//liquid.internal.h .. macht keinen sinn das zu importieren .. generiert zuviele probleme .. 
//daher den teil der gebraucht wird rüber genommen
#if HAVE_FFTW3_H && !defined LIQUID_FFTOVERRIDE
#   include <fftw3.h>
#   define FFT_PLAN             fftwf_plan
#   define FFT_CREATE_PLAN      fftwf_plan_dft_1d
#   define FFT_DESTROY_PLAN     fftwf_destroy_plan
#   define FFT_EXECUTE          fftwf_execute
#   define FFT_DIR_FORWARD      FFTW_FORWARD
#   define FFT_DIR_BACKWARD     FFTW_BACKWARD
#   define FFT_METHOD           FFTW_ESTIMATE
#   define FFT_MALLOC           fftwf_malloc
#   define FFT_FREE             fftwf_free
#else
#   define FFT_PLAN             fftplan
#   define FFT_CREATE_PLAN      fft_create_plan
#   define FFT_DESTROY_PLAN     fft_destroy_plan
#   define FFT_EXECUTE          fft_execute
#   define FFT_DIR_FORWARD      LIQUID_FFT_FORWARD
#   define FFT_DIR_BACKWARD     LIQUID_FFT_BACKWARD
#   define FFT_METHOD           0
#   define FFT_MALLOC           malloc
#   define FFT_FREE             free
#endif
