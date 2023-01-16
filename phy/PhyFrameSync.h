#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <complex>
#include <iostream>
//#include <complex.h>

#include "liquid.h"
//#include "liquid.internal.h"

#include "phy/PhyDefinitions.h"
#include "util/log.h"


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


class PhyFrameSync {
public:

    PhyFrameSync();

    PhyFrameSync(unsigned int M,
                 unsigned int cp_len,
                 unsigned int taper_len);

    ~PhyFrameSync();


    void execute(liquid_float_complex sample);


    uint64_t m_currentSampleTimestamp;

protected:



private:
    unsigned int m_M;                               // number of subcarriers
    unsigned int m_M2;                               // number of subcarriers div by 2
    unsigned int m_cp_len;                          // cyclic prefix length
    unsigned int m_taper_len;                       // taper length
    unsigned char * m_subcarrier_allocation_STS;    // subcarrier allocation (null, pilot, data)
    unsigned char * m_subcarrier_allocation_LTS;    // subcarrier allocation (null, pilot, data)

    // constants
    unsigned int m_M_null;    // number of null subcarriers
    unsigned int m_M_pilot;   // number of pilot subcarriers
    unsigned int m_M_data;    // number of data subcarriers
    unsigned int m_M_STS;      // number of enabled subcarriers in STS
    unsigned int m_M_LTS;      // number of enabled subcarriers in LTS


    // scaling factors
    float m_g_data;           // data symbols gain
    float m_g_STS;            // STS training symbols gain
    float m_g_LTS;            // LTS training symbols gain

    // transform object
    FFT_PLAN m_fft;           // ifft object
    
    liquid_float_complex *m_X;      // frequency-domain buffer
    liquid_float_complex *m_x;      // time-domain buffer
    windowcf m_input_buffer;  // input sequence buffer

    // STS and LTS sequences
    liquid_float_complex *m_STS;     // STS sequence (freq) //_S0
    liquid_float_complex *m_sts;     // STS sequence (time) //_s0
    liquid_float_complex *m_LTS;     // LTS sequence (freq)
    liquid_float_complex *m_lts;     // LTS sequence (time)


    // gain
    float m_g0;               // nominal gain (coarse initial estimate)
    liquid_float_complex *m_gain_STSa;    // complex subcarrier gain estimate, STS[a]
    liquid_float_complex *m_gain_STSb;    // complex subcarrier gain estimate, STS[b]
    liquid_float_complex *m_gain_LTS;     // complex subcarrier gain estimate, LTS
    liquid_float_complex *m_gain_G;      // complex subcarrier gain estimate
    liquid_float_complex *m_gain_B;      // subcarrier phase rotation due to backoff
    liquid_float_complex *m_gain_R;      // 

    // synchronizer objects
    nco_crcf m_nco_rx;        // numerically-controlled oscillator
    float m_phi_prime;        // ...
    float m_p1_prime;         // filtered pilot phase slope

// #if ENABLE_SQUELCH
//     // coarse signal detection
//     float m_squelch_threshold;
//     int m_squelch_enabled;
// #endif

    // timing
    unsigned int m_timer;               // input sample timer
    unsigned int m_num_symbols;         // symbol counter
    unsigned int m_backoff;             // sample timing backoff

    liquid_float_complex m_s_hat_0;     // first S0 symbol metrics estimate
    liquid_float_complex m_s_hat_1;     // second S0 symbol metrics estimate

    // detection thresholds
    float m_STS_detect_thresh;   // detection threshold, nominally 0.35 // to be checked
    float m_STS_sync_thresh;     // long symbol threshold, nominally 0.30

    int init_STS_sctype(); 
    int init_LTS_sctype();

    int validate_sctype_STS(); // ofdmframe_validate_sctype(q->p, q->M, &q->M_null, &q->M_pilot, &q->M_data)

    int init_STS(); //ofdmframe_init_S0(q->p, q->M, q->S0, q->s0, &q->M_S0);

    int reset_parameters();

    int execute_seekSTS();

    int estimate_gain_STS(liquid_float_complex *rc, liquid_float_complex *G);

    int STS_metrics(liquid_float_complex *G, liquid_float_complex &s );


};
