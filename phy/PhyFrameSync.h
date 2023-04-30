#pragma once

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <cassert>
#include <complex>
#include <iostream>

#include <liquid.h>
//#include "liquid.internal.h"

#include "phy/PhyDefinitions.h"
#include "util/log.h"
#include "PhyIQDebug.h"



class PhyFrameSync {
public:

    PhyFrameSync();

    PhyFrameSync(unsigned int M,
                 unsigned int cp_len,
                 unsigned int taper_len);

    ~PhyFrameSync();


    void execute(liquid_float_complex sample);


    uint64_t m_currentSampleTimestamp;


    void setIQDebug(PhyIQDebugPtr iqd) { m_iqdebug = iqd; }

protected:



private:

    // statemachine
    // receiver sync state
    enum {
        FRAMESYNC_STATE_DETECT_STS=0,   // seek initial STS
        FRAMESYNC_STATE_SYNC_STS,       // (re)sync STS
        FRAMESYNC_STATE_STS_0,          // seek first STS short sequence
        FRAMESYNC_STATE_STS_1,          // seek second STS short sequence
        FRAMESYNC_STATE_LTS,            // seek LTS long sequence
        FRAMESYNC_STATE_RXSYMBOLS       // receive payload symbols
    } m_frameSyncState;

    uint16_t m_wait = 0;


    // symbol information
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

    liquid_float_complex m_s_hat_d;     // detect S0 symbol metrics estimate
    liquid_float_complex m_s_hat_0;     // first S0 symbol metrics estimate
    liquid_float_complex m_s_hat_1;     // second S0 symbol metrics estimate

    // detection thresholds
    float m_STS_detect_upper_thresh;   // detection threshold, nominally 0.35 // to be checked
    float m_STS_detect_lower_thresh;
    float m_STS_sync_thresh;     // long symbol threshold, nominally 0.30

    bool m_STS_detect_hit_upper_tresh;  // set to true when in STS detection the upper limit is first hit (i.e. we need to backoff a bit)


    int m_sync_STS_count;


    int init_STS_sctype(); 
    int init_LTS_sctype();

    int validate_sctype_STS(); // ofdmframe_validate_sctype(q->p, q->M, &q->M_null, &q->M_pilot, &q->M_data)

    int init_STS(); //ofdmframe_init_S0(q->p, q->M, q->S0, q->s0, &q->M_S0);

    int reset_parameters();

    int execute_detect_STS();
    int execute_sync_STS();

    int execute_sync_STSa();
    int execute_sync_STSb();

    int estimate_gain_STS(liquid_float_complex *rc, liquid_float_complex *G);

    int STS_metrics(liquid_float_complex *G, liquid_float_complex &s );



    PhyIQDebugPtr m_iqdebug;

};
