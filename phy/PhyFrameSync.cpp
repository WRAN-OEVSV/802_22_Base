

#include "phy/PhyFrameSync.h"


PhyFrameSync::PhyFrameSync() {
    LOG_PHY_INFO("PhyFrameSync::PhyFrameSync() called");
}

PhyFrameSync::PhyFrameSync(unsigned int M,
                           unsigned int cp_len,
                           unsigned int taper_len) 
    : m_M{M}, m_cp_len{cp_len}, m_taper_len{taper_len} {

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

#if 1
    memset(m_gain_STSa, 0x00, m_M*sizeof(liquid_float_complex));
    memset(m_gain_STSb, 0x00, m_M*sizeof(liquid_float_complex));
    memset(m_gain_G ,  0x00, m_M*sizeof(liquid_float_complex));
    memset(m_gain_B,   0x00, m_M*sizeof(liquid_float_complex));
#endif

    // timing backoff
    m_backoff = m_cp_len < 2 ? m_cp_len : 2;
    float phi = (float)(m_backoff)*2.0f*M_PI/(float)(m_M);
    unsigned int i;
    for (i=0; i<m_M; i++)
        m_gain_B[i] = cosf(i*phi) + 1.0if * sinf(i*phi); //liquid_cexpjf_std(i*phi);   


    //#define liquid_cexpjf(THETA) (cosf(THETA) + _Complex_I*sinf(THETA))

    // 
    // synchronizer objects
    //

    // numerically-controlled oscillator
    m_nco_rx = nco_crcf_create(LIQUID_NCO);

#if PHY_ENABLE_SQUELCH
    // coarse detection
    q->squelch_threshold = -25.0f;
    q->squelch_enabled = 0;
#endif

    // // reset object
    reset_parameters();

}


PhyFrameSync::~PhyFrameSync() {

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
    m_phi_prime = 0.0f;
    m_p1_prime = 0.0f;

    // set thresholds (increase for small number of subcarriers)
    m_STS_detect_thresh = (m_M > 44) ? 0.35f : 0.35f + 0.01f*(44 - m_M);
    m_STS_sync_thresh   = (m_M > 44) ? 0.30f : 0.30f + 0.01f*(44 - m_M);

    // // reset state
    // _q->state = OFDMFRAMESYNC_STATE_SEEKPLCP;
    return 0;


}


int PhyFrameSync::init_STS() {

    //

    unsigned int i;

    // // @todo das muss nochmal angeschaut werden - da wir eigentlich mit m=8 fahren sollten bei m_M = 1024
    // // damit eine 4x 256 bit sequenz entsteht - wir nehmen jetzt mal nur 256 bit von ms
    // unsigned int ms_m = 9;                     // _m : generator polynomial length, sequence length is (2^m)-1
    // unsigned int ms_g = 0b0110110101;          // _g : generator polynomial, starting with most-significant bit
    // unsigned int ms_a = 0b1111111111;          // _a : initial shift register state, default: 000...001


    // // generate m-sequence generator object
    // msequence ms = msequence_create(ms_m, ms_g, ms_a);

    unsigned int sts_sequence[256] = 
        { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 1, 0, 
          1, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 1, 
          0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 
          1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 0, 
          0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 
          0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 
          0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 
          0, 0, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 
          1, 0, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1, 1, 1, 1, 0, 
          0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 
          0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 
          1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 0, 
          0, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 
          1, 0, 1, 0, 1, 1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 1, 
          0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 0, 0, 
          1, 1, 1, 0, 1, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 1 };


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

    std::cout << "check check" << std::endl;
    for(i=0; i<m_M; i++)
        std::cout << m_STS[i] << " ";
    std::cout  << std::endl;


    // destroy objects
 //   msequence_destroy(ms);

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
    return LIQUID_OK;

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

    windowcf_push(m_input_buffer,sample);

    execute_seekSTS();

}

int PhyFrameSync::execute_seekSTS() {

    m_timer++;

    if (m_timer < m_M)
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
 //   s_hat *= g;

    float tau_hat  = std::arg(s_hat) * (float)(m_M2) / (2*M_PI);

    // LOG_PHY_DEBUG("PhyFrameSync::execute_seekSTS() gain={}, rssi={}, s_hat={} <{}>, tau_hat={}",
    //         sqrt(g),
    //         -10*log10(g),
    //         std::abs(s_hat), std::arg(s_hat),
    //         tau_hat);

    std::cout << "grepgrepgrep," << std::abs(s_hat) << "," << std::arg(s_hat) << "," << tau_hat << std::endl;


    // save gain (permits dynamic invocation of get_rssi() method)
    m_g0 = g;

    // 
    if (std::abs(s_hat) > m_STS_detect_thresh) {

        int dt = (int)roundf(tau_hat);
        // set timer appropriately...
        m_timer = (m_M + dt) % (m_M2);
        m_timer += m_M; // add delay to help ensure good S0 estimate
  //      _q->state = OFDMFRAMESYNC_STATE_PLCPSHORT0;

    }
    return LIQUID_OK;
}

int PhyFrameSync::estimate_gain_STS(liquid_float_complex *rc, liquid_float_complex *G) {

    // move input array into fft input buffer
    memmove(m_x, rc, (m_M)*sizeof(liquid_float_complex));

    // compute fft, storing result into _q->X
    FFT_EXECUTE(m_fft);
    
    // compute gain, ignoring NULL subcarriers
    unsigned int i;
    float gain = sqrtf(m_M_STS) / (float)(m_M);

    for (i=0; i< m_M; i++)
        G[i] = 0.0f;
    
    unsigned int n = 0;

    for (i=0; i< (m_M/4); i++) {
        n = ((i+1)*4)-1;
        G[n] = m_X[n] * conj(m_STS[n]);
  //      G[n] *= gain;
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
//        s_hat += G[(i+4)]*conj(G[i]);
        s_hat += G[i]*conj(G[(i+4)]);
    }
 //   s_hat /= m_M_STS; // normalize output
    
   // LOG_PHY_DEBUG("PhyFrameSync::STS_metrics() - s_hat {} {} m_M_STS {}", std::abs(s_hat), std::arg(s_hat), m_M_STS);
    // set output values
    s = s_hat;
    return 0;
}
