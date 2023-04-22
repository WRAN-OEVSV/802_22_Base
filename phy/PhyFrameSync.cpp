

#include "phy/PhyFrameSync.h"


PhyFrameSync::PhyFrameSync() {
    LOG_PHY_INFO("PhyFrameSync::PhyFrameSync() called");
}

PhyFrameSync::PhyFrameSync(unsigned int M,
                           unsigned int cp_len,
                           unsigned int taper_len) 
    : m_M{M}, m_cp_len{cp_len}, m_taper_len{taper_len}, m_frameSyncState{FRAMESYNC_STATE_DETECT_STS} {

    LOG_PHY_INFO("PhyFrameSync::PhyFrameSync(M,cp,taper) called {} {} {} ", M, cp_len, taper_len );

    using namespace std::complex_literals;

    // derived values
    m_M2 = m_M/2;

    

    m_subcarrier_allocation_STS = (unsigned char*) malloc((m_M)*sizeof(unsigned char)); 
    m_subcarrier_allocation_LTS = (unsigned char*) malloc((m_M)*sizeof(unsigned char)); 

    // init_STS_sctype();
    // init_LTS_sctype();


    // if(validate_sctype_STS())
    //     LOG_PHY_ERROR("error validation of subcarrier allocation");


    // create transform object
    m_X = (liquid_float_complex*) FFT_MALLOC((m_M)*sizeof(liquid_float_complex));
    m_x = (liquid_float_complex*) FFT_MALLOC((m_M)*sizeof(liquid_float_complex));
    m_fft = FFT_CREATE_PLAN(m_M, m_x, m_X, FFT_DIR_FORWARD, FFT_METHOD);
 
    // create input buffer the length of the transform
    m_input_buffer = windowcf_create(m_M + m_cp_len);


    // allocate memory for PLCP arrays
    m_STS = (liquid_float_complex*) malloc((m_M)*sizeof(liquid_float_complex)); // q->S0 frequency domain
    m_sts = (liquid_float_complex*) malloc((m_M)*sizeof(liquid_float_complex)); // q->s0 time domain
    // q->S1 = (float complex*) malloc((q->M)*sizeof(float complex));
    // q->s1 = (float complex*) malloc((q->M)*sizeof(float complex));
    init_STS();
    // ofdmframe_init_S1(q->p, q->M, q->S1, q->s1, &q->M_S1);

    // compute scaling factor
    m_g_data  = sqrtf(m_M) / sqrtf(m_M_pilot + m_M_data);
    m_g_STS   = sqrtf(m_M) / sqrtf(m_M_STS);
    m_g_LTS   = sqrtf(m_M) / sqrtf(m_M_LTS);

    // gain
    m_g0 = 1.0f;
    m_gain_STSa = (liquid_float_complex*) malloc((m_M)*sizeof(liquid_float_complex));
    m_gain_STSb = (liquid_float_complex*) malloc((m_M)*sizeof(liquid_float_complex));
    m_gain_G   = (liquid_float_complex*) malloc((m_M)*sizeof(liquid_float_complex));
    m_gain_B   = (liquid_float_complex*) malloc((m_M)*sizeof(liquid_float_complex));
    m_gain_R   = (liquid_float_complex*) malloc((m_M)*sizeof(liquid_float_complex));

    memset(m_gain_STSa, 0x00, m_M*sizeof(liquid_float_complex));
    memset(m_gain_STSb, 0x00, m_M*sizeof(liquid_float_complex));
    memset(m_gain_G ,   0x00, m_M*sizeof(liquid_float_complex));
    memset(m_gain_B,    0x00, m_M*sizeof(liquid_float_complex));

    // timing backoff
    m_backoff = m_cp_len < 2 ? m_cp_len : 2;
    float phi = (float)(m_backoff)*2.0f*M_PI/(float)(m_M);
    unsigned int i;
    for (i=0; i<m_M; i++)
        m_gain_B[i] = cosf(i*phi) + 1.0if * sinf(i*phi); //liquid_cexpjf_std(i*phi);   
        //#define liquid_cexpjf(THETA) (cosf(THETA) + _Complex_I*sinf(THETA))


    // thresholds were derived with octave analyzing the raw s_hat data
    // this probably needs to be rqchecked on a real radio link
    // @todo check with real radio link
    m_STS_detect_lower_thresh = PHY_STS_DETECT_LWR_THR;
    m_STS_detect_upper_thresh = PHY_STS_DETECT_UPR_THR;
    m_STS_sync_thresh = 1400;

    m_STS_detect_hit_upper_tresh = false;

    // synchronizer objects

    // numerically-controlled oscillator
    m_nco_rx = nco_crcf_create(LIQUID_NCO);             // creates NCO with theta = 0 and d_theta = 0

// #if PHY_ENABLE_SQUELCH
//     // coarse detection
//     q->squelch_threshold = -25.0f;
//     q->squelch_enabled = 0;
// #endif

    // // reset object
    reset_parameters();

}


PhyFrameSync::~PhyFrameSync() {
    LOG_PHY_INFO("PhyFrameSync::~PhyFrameSync() called");
    // free up all stuff

}



int PhyFrameSync::reset_parameters() {


    // reset synchronizer objects
    nco_crcf_reset(m_nco_rx);

    // reset timers
    m_timer = 0;
    m_num_symbols = 0;
    m_s_hat_0 = 0.0f;
    m_s_hat_1 = 0.0f;
    m_s_hat_d = 0.0f;
    m_phi_prime = 0.0f;
    m_p1_prime = 0.0f;

    // set thresholds (increase for small number of subcarriers)
    m_STS_detect_lower_thresh = PHY_STS_DETECT_LWR_THR;
    m_STS_detect_upper_thresh = PHY_STS_DETECT_UPR_THR; // (m_M > 44) ? 0.35f : 0.35f + 0.01f*(44 - m_M);
    m_STS_sync_thresh   = 1400; // (m_M > 44) ? 0.30f : 0.30f + 0.01f*(44 - m_M);

    m_STS_detect_hit_upper_tresh = false;

    // // reset state
    m_frameSyncState = FRAMESYNC_STATE_DETECT_STS;
    return 0;


}

/**
 * @brief initalize the STS seqence in frequency and time domain
 * 
 * @return int 
 */
int PhyFrameSync::init_STS() {

    unsigned int i;

    // @todo - check msequence - this somehow does not work
    // // @todo das muss nochmal angeschaut werden - da wir eigentlich mit m=8 fahren sollten bei m_M = 1024
    // // damit eine 4x 256 bit sequenz entsteht - wir nehmen jetzt mal nur 256 bit von ms
    // unsigned int ms_m = 9;                     // _m : generator polynomial length, sequence length is (2^m)-1
    // unsigned int ms_g = 0b0110110101;          // _g : generator polynomial, starting with most-significant bit
    // unsigned int ms_a = 0b1111111111;          // _a : initial shift register state, default: 000...001


    // // generate m-sequence generator object
    // msequence ms = msequence_create(ms_m, ms_g, ms_a);

    // unsigned int sts_sequence[256] = 
    //     { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 1, 0, 
    //       1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 1, 
    //       0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 
    //       1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 0, 
    //       0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 
    //       0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 
    //       0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 
    //       0, 0, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 
    //       1, 0, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1, 1, 1, 1, 0, 
    //       0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 
    //       0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 
    //       1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 
    //       0, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 
    //       1, 0, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 1, 
    //       0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 0, 0, 
    //       1, 1, 1, 0, 1, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 1 };

    unsigned int sts_sequence[256] = PHY_STS_SEQUENCE;                  // defined in PhyDefinitions.h


    unsigned int s;
    unsigned int M_STS = 0;


    // init all to 0
    for (i=0; i<(m_M); i++)
        m_STS[i] = 0.0f;

    // short sequence
    for (i=0; i<(m_M/4); i++) {
        // generate symbol
        //s = msequence_generate_symbol(ms,1);
        // s = msequence_generate_symbol(ms,3) & 0x01;
        s = sts_sequence[i];

        m_STS[((i+1)*4)-1] = s ? 1.0f : -1.0f;
        M_STS++;
    }


    // do an fftshift of m_STS of the STS Sync symbol as the sender (for the test gnuradio) is
    // shifting the sync also 
    // @todo check for futur if we still do this - further the DC subcarriers should be checked again
    // @todo as the generated sequence is not 100% what the standard describes
    liquid_float_complex tmp;

    for (i=0; i<m_M2; i++) {
        tmp = m_STS[i];
        m_STS[i] = m_STS[i+m_M2];
        m_STS[i+m_M2] = tmp;
    }

    // std::cout << "check check" << std::endl;
    // for(i=0; i<m_M; i++)
    //     std::cout << m_STS[i] << " ";
    // std::cout  << std::endl;


    // destroy objects
    // msequence_destroy(ms);

    // ensure at least one subcarrier was enabled
    if (M_STS == 0) {
        LOG_PHY_ERROR("PhyFrameSync::init_STS(), no subcarriers enabled; check allocation");
        return 1;
    }

    // set return value(s)
    m_M_STS = M_STS;

    // run inverse fft to get time-domain sequence
    fft_run(m_M, m_STS, m_sts, LIQUID_FFT_BACKWARD, 0);

    // normalize time-domain sequence level
    float g = 1.0f / sqrtf(M_STS);
    for (i=0; i<m_M; i++)
        m_sts[i] *= g;

    return 0;
}

int PhyFrameSync::validate_sctype_STS() {

    // clear counters
    unsigned int M_null  = 0;
    unsigned int M_pilot = 0;
    unsigned int M_data  = 0;

    unsigned int i;
    for (i=0; i<m_M; i++) {
        // update appropriate counters
        if (m_subcarrier_allocation_STS[i] == OFDMFRAME_SCTYPE_NULL)
            M_null++;
        else if (m_subcarrier_allocation_STS[i] == OFDMFRAME_SCTYPE_PILOT)
            M_pilot++;
        else if (m_subcarrier_allocation_STS[i] == OFDMFRAME_SCTYPE_DATA)
            M_data++;
        else {
            LOG_PHY_ERROR("PhyFrameSynce::validate_sctype_STS(), invalid subcarrier type {}", m_subcarrier_allocation_STS[i]);
            return 1;
        }
    }

    // set outputs
    m_M_null  = M_null;
    m_M_pilot = M_pilot;
    m_M_data  = M_data;

    return 0;

}


int PhyFrameSync::init_STS_sctype() {

    // copy 802.22 standard defined STS pattern into m_subcarrier_allocation_STS

    return 0;
}


int PhyFrameSync::init_LTS_sctype() {

    // copy 802.22 standard defined STS pattern into m_subcarrier_allocation_LTS


    return 0;
}



void PhyFrameSync::execute(liquid_float_complex sample) {


    // @todo - NCO must be handled in PhyThread as it is needed for all samples not only STS/LTS but also data samples

    // correct for carrier frequency offset
    if (m_frameSyncState != FRAMESYNC_STATE_DETECT_STS) {

        // Rotate input vector down by NCO angle, y = x exp{-j theta}
        nco_crcf_mix_down(m_nco_rx, sample, &sample);
        nco_crcf_step(m_nco_rx);
    }



    windowcf_push(m_input_buffer,sample);


    // @note - not to forget!!
    // we must consider two different "phases" - there is the "inital" search for the start of the frame structure
    // and after this sync has happend it must switch to "periodic" re-sync; on the following 802.22 super and non-super frames


    switch (m_frameSyncState)
    {
    case FRAMESYNC_STATE_DETECT_STS:
        execute_detect_STS();
        break;

    case FRAMESYNC_STATE_SYNC_STS:
        execute_sync_STS();
        break;

    case FRAMESYNC_STATE_STS_0:
        execute_sync_STSa();
        break;

    case FRAMESYNC_STATE_STS_1:
        execute_sync_STSb();
        break;

    case FRAMESYNC_STATE_LTS:
        if(m_timer > 0)
            m_timer--;              // finish STS frame
        else {
            m_frameSyncState = FRAMESYNC_STATE_RXSYMBOLS;
        }
        break;
    case FRAMESYNC_STATE_RXSYMBOLS:
        if(m_wait > (14*1280)) {
            m_wait = 0;
            m_timer = 0;
            m_frameSyncState = FRAMESYNC_STATE_SYNC_STS;
            std::cout << m_currentSampleTimestamp << std::endl;
        } else {
            m_wait++;
        }
        break;
    default:
        m_frameSyncState = FRAMESYNC_STATE_DETECT_STS;
//        m_frameSyncState = FRAMESYNC_STATE_STS_0;
        break;

    }
    

}

int PhyFrameSync::execute_detect_STS() {

    m_timer++;

    if (m_timer < (m_M/4))             // smaller search window
        return 0;

    // reset timer
    m_timer = 0;

    //
    liquid_float_complex *rc;
    windowcf_read(m_input_buffer, &rc);

    // estimate gain
    unsigned int i;
    // start with a reasonably small number to avoid divide-by-zero warning
    float g = 1.0e-9f;
    for (i= m_cp_len; i<m_M + m_cp_len; i++) {
        // compute |rc[i]|^2 efficiently
        g += rc[i].real()*rc[i].real() + rc[i].imag()*rc[i].imag();
    }
    g = (float)(m_M) / g;

// #if ENABLE_SQUELCH
//     // TODO : squelch here
//     if ( -10*log10f( sqrtf(g) ) < _q->squelch_threshold &&
//          _q->squelch_enabled)
//     {
//         printf("squelch\n");
//         return LIQUID_OK;
//     }
// #endif

    // estimate S0 gain
    estimate_gain_STS(&rc[m_cp_len], m_gain_STSa);

    liquid_float_complex s_hat;
    STS_metrics(m_gain_STSa, s_hat);
    s_hat *= g;

    float tau_hat  = std::arg(s_hat) * (float)(m_M2) / (2*M_PI);

    // LOG_PHY_DEBUG("PhyFrameSync::execute_detect_STS() gain={}, rssi={}, s_hat={} <{}>, tau_hat={}",
    //         sqrt(g),
    //         -10*log10(g),
    //         std::abs(s_hat), std::arg(s_hat),
    //         tau_hat);

 //   std::cout << "grepgrepgrep," << m_currentSampleTimestamp << "," << std::abs(s_hat) << "," << std::arg(s_hat) << "," << tau_hat << "," << m_timer << std::endl;

    std::cout << "g " << g << "rssi " << -10*log10(g) << std::endl;

    // save gain (permits dynamic invocation of get_rssi() method)
    m_g0 = g;

    // 
    if (std::abs(s_hat) > m_STS_detect_lower_thresh) {
    // if (std::abs(s_hat) > 0) {
    //     m_timer = 2*m_M;

        int dt = (int)roundf(tau_hat);
        std::cout << "grepgrepgrepZ," << m_currentSampleTimestamp << "," << std::abs(s_hat) << "," << std::arg(s_hat) << "," << tau_hat << "," << dt << std::endl;


        if(std::abs(s_hat) < m_STS_detect_upper_thresh) {

            // if(std::abs(s_hat) > std::abs(m_s_hat_d))
            
            if(!m_STS_detect_hit_upper_tresh) {         

                std::cout << "framestart (roughly)" << (m_currentSampleTimestamp - 1024 + (512- dt)) << std::endl;

                // set timer appropriately...
                m_timer = (m_M + dt) % (m_M2);
                m_timer += m_M; // add delay to help ensure good S0 estimate


                // we are on the raising slope of detection i.e. we are at the beginning of the STS symbol
                m_frameSyncState = FRAMESYNC_STATE_STS_0;
            } else {
                std::cout << "coming from upper" << std::endl;
                // we hit the symbol in the middel 
                // we forward a bit and let the STS_DETECT run
                m_timer = 0;
//                m_STS_detect_hit_upper_tresh = false;
                m_frameSyncState = FRAMESYNC_STATE_DETECT_STS;
            }

            
        } else {
            std::cout << "hit upper" << std::endl;
            m_STS_detect_hit_upper_tresh = true;
            m_timer = 0;
        }
    } else {
        // when we hit a non STS frame we reset the upper threshold hit
        std::cout << "below lower" << std::endl;
        m_STS_detect_hit_upper_tresh = false;
    }
    return 0;
}





int PhyFrameSync::execute_sync_STS() {

    m_timer++;

    // at this point we should be in the CP of the STS frame (after the inital lock)
    // we move towards the lower threshold in sample chunks
    if (m_timer < 31) {
        if(m_timer == 1) std::cout << ":";
        return 0;
    }
    //std::cout << std::endl;

    // reset timer
    m_timer = 0;

    //
    liquid_float_complex *rc;
    windowcf_read(m_input_buffer, &rc);

    // estimate gain
    unsigned int i;
    // start with a reasonably small number to avoid divide-by-zero warning
    float g = 1.0e-9f;
    for (i= m_cp_len; i<m_M + m_cp_len; i++) {
        // compute |rc[i]|^2 efficiently
        g += rc[i].real()*rc[i].real() + rc[i].imag()*rc[i].imag();
    }
    g = (float)(m_M) / g;

    // estimate S0 gain
    estimate_gain_STS(&rc[m_cp_len], m_gain_STSa);

    liquid_float_complex s_hat;
    STS_metrics(m_gain_STSa, s_hat);
    s_hat *= g;

    float tau_hat  = std::arg(s_hat) * (float)(m_M2) / (2*M_PI);

    // LOG_PHY_DEBUG("PhyFrameSync::execute_detect_STS() gain={}, rssi={}, s_hat={} <{}>, tau_hat={}",
    //         sqrt(g),
    //         -10*log10(g),
    //         std::abs(s_hat), std::arg(s_hat),
    //         tau_hat);

    // save gain (permits dynamic invocation of get_rssi() method)
    m_g0 = g;


    if (std::abs(s_hat) > m_STS_detect_lower_thresh) {
        int dt = (int)roundf(tau_hat);
        std::cout << "\ngrepgrepgrepX," << m_currentSampleTimestamp << "," << std::abs(s_hat) << "," << std::arg(s_hat) << "," << tau_hat << "," << dt << std::endl;

        m_timer = m_M; // add delay to help ensure good S0 estimate

        // we are on the raising slope of detection i.e. we are at the beginning of the STS symbol
        m_frameSyncState = FRAMESYNC_STATE_STS_0;
    } 

    return 0;
}








int PhyFrameSync::execute_sync_STSa()
{
    m_timer++;

    if (m_timer < m_M2)
        return 0;

    // reset timer
    m_timer = 0;

    //
    liquid_float_complex *rc;
    windowcf_read(m_input_buffer, &rc);

    // TODO : re-estimate nominal gain

    // estimate STS gain
    estimate_gain_STS(&rc[m_cp_len], m_gain_STSa);

    liquid_float_complex s_hat;
    STS_metrics(m_gain_STSa, s_hat);
    s_hat *= m_g0;

    m_s_hat_0 = s_hat;

// #if DEBUG_OFDMFRAMESYNC_PRINT
//     float tau_hat  = cargf(s_hat) * (float)(_q->M2) / (2*M_PI);
//     printf("********** S0[0] received ************\n");
//     printf("    s_hat   :   %12.8f <%12.8f>\n", cabsf(s_hat), cargf(s_hat));
//     printf("  tau_hat   :   %12.8f\n", tau_hat);
// #endif

// // TODO : also check for phase of s_hat (should be small)
// if (std::abs(s_hat) < std::abs(m_s_hat_d)) {
//     // false alarm
//     LOG_PHY_ERROR("false alarm STS[0]");
//     reset_parameters();
//     return 1;
// }

//    std::cout << "grepgrepgrep_a," << m_currentSampleTimestamp << "," << std::abs(s_hat) << "," << std::arg(s_hat) << ",0," << m_timer << std::endl;

    m_frameSyncState = FRAMESYNC_STATE_STS_1;
    return 0;
}

int PhyFrameSync::execute_sync_STSb()
{
    //printf("t = %u\n", _q->timer);
    m_timer++;

    if (m_timer < m_M2)
        return 0;

    // reset timer
    // m_timer = m_M + m_cp_len - m_backoff;

    //
    liquid_float_complex *rc;
    windowcf_read(m_input_buffer, &rc);

    // estimate STS gain
    estimate_gain_STS(&rc[m_cp_len], m_gain_STSb);

    liquid_float_complex s_hat;
    STS_metrics(m_gain_STSb, s_hat);
    s_hat *= m_g0;

    m_s_hat_1 = s_hat;

// #if DEBUG_OFDMFRAMESYNC_PRINT
//     float tau_hat  = cargf(s_hat) * (float)(_q->M2) / (2*M_PI);
//     printf("********** S0[1] received ************\n");
//     printf("    s_hat   :   %12.8f <%12.8f>\n", cabsf(s_hat), cargf(s_hat));
//     printf("  tau_hat   :   %12.8f\n", tau_hat);

//     // new timing offset estimate
//     tau_hat  = cargf(_q->s_hat_0 + _q->s_hat_1) * (float)(_q->M2) / (2*M_PI);
//     printf("  tau_hat * :   %12.8f\n", tau_hat);

//     printf("**********\n");
// #endif

    // re-adjust timer accordingly
    float tau_prime = std::arg(m_s_hat_0 + m_s_hat_1) * (float)(m_M2) / (2*M_PI);

    m_timer = 256 - (int)roundf(tau_prime);



    if(std::abs(m_s_hat_1) < (std::abs(m_s_hat_0)-0.5)) {
        // we hit the frame at the very end on the first bunch of smaples received by the SDR
        std::cout << "STSb hit coming from upper" << std::endl;
        std::cout << "STSb " << m_currentSampleTimestamp << "," << std::abs(m_s_hat_0) << "," << std::arg(m_s_hat_1)<< ","  << tau_prime << std::endl;

        m_STS_detect_hit_upper_tresh = true;
        m_timer = 0;
        m_frameSyncState = FRAMESYNC_STATE_DETECT_STS;
        return 1;
    }


// #if 0
//     if (cabsf(s_hat) < 0.3f) {
// #if DEBUG_OFDMFRAMESYNC_PRINT
//         printf("false alarm S0[1]\n");
// #endif
//         // false alarm
//         ofdmframesync_reset(_q);
//         return;
//     }
// #endif

     unsigned int i;
// #if 0
//     float complex g_hat = 0.0f;
//     for (i=0; i<_q->M; i++)
//         g_hat += _q->G0b[i] * conjf(_q->G0a[i]);

//     // compute carrier frequency offset estimate using freq. domain method
//     float nu_hat = 2.0f * cargf(g_hat) / (float)(_q->M);
// #else
    // compute carrier frequency offset estimate using ML method
    liquid_float_complex t0 = 0.0f;
    for (i=0; i<m_M2; i++) {
        t0 += conj(rc[i]) * m_sts[i] * rc[i+m_M2] * conj(m_sts[i+m_M2]);
    }
    float nu_hat = std::arg(t0) / (float)(m_M2);
// #endif

// #if DEBUG_OFDMFRAMESYNC_PRINT
//     printf("   nu_hat   :   %12.8f\n", nu_hat);
// #endif

    std::cout << "grepgrepgrep_b," << m_currentSampleTimestamp << "," << std::abs(s_hat) << "," << std::arg(s_hat) << "," << tau_prime << "," << m_timer << "," << nu_hat << std::endl;

    // NICHT SICHER OB DAS STIMMT MIT DEM FRAME START ....
    std::cout << "framestart (roughly)" << (m_currentSampleTimestamp - 1024 + (0 - (int)roundf(tau_prime))) << std::endl;



    // set NCO frequency
    nco_crcf_set_frequency(m_nco_rx, nu_hat);

    m_frameSyncState = FRAMESYNC_STATE_LTS;
    return 0;
}






int PhyFrameSync::estimate_gain_STS(liquid_float_complex *rc, liquid_float_complex *G) {

    // move input array into fft input buffer
    memmove(m_x, rc, (m_M)*sizeof(liquid_float_complex));

    // compute fft, storing result into _q->X
    FFT_EXECUTE(m_fft);
    
    // compute gain, ignoring NULL subcarriers
    unsigned int i;

    //value derived through checking with octave - value smaller than 0.55f created kind of a distortion
    //original - sqrtf(m_M_STS) / (float)(m_M);
    float gain = 0.055f; 
    

//     for (i=0; i< m_M; i++)
//         G[i] = 0.0f;
    
//     unsigned int n = 0;

//     for (i=0; i< (m_M/4); i++) {
//         n = ((i+1)*4)-1;
//         G[n] = m_X[n] * conj(m_STS[n]);
//   //      G[n] *= gain;
//     }

    // @todo - optimize to not touch all entries 
    for (i=0; i<m_M; i++) {
        G[i] = m_X[i] * conj(m_STS[i]);
        G[i] *= gain;
    }

    return 0;
}


int PhyFrameSync::STS_metrics(liquid_float_complex *G, liquid_float_complex &s ) {

    // timing, carrier offset correction
    unsigned int i;
    liquid_float_complex s_hat = 0.0f;

    // for(i=0; i < m_M; i++)
    //     std::cout << G[i] << " ";
    // std::cout << std::endl;

    // compute timing estimate, accumulate phase difference across
    // gains on subsequent pilot subcarriers (note that all the odd
    // subcarriers are NULL)
    for (i=3; i<(m_M-4); i+=4) {
        s_hat += G[(i+4)]*conj(G[i]);
//        s_hat += G[i]*conj(G[(i+4)]);
    }

    //the normalizing as done in the liquid ofdmframesync does create problems 
    s_hat /= m_M_STS; // normalize output
    
    // LOG_PHY_DEBUG("PhyFrameSync::STS_metrics() - s_hat {} {} m_M_STS {}", std::abs(s_hat), std::arg(s_hat), m_M_STS);
    // set output values
    s = s_hat;
    return 0;
}
