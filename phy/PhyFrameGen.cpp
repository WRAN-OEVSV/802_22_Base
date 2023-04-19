

#include "phy/PhyFrameGen.h"



PhyFrameGen::PhyFrameGen() {
    LOG_PHY_INFO("PhyFrameGen::PhyFrameGen() called");
}


PhyFrameGen::PhyFrameGen(unsigned int M,
                         unsigned int cp_len,
                         unsigned int taper_len) 
    : m_M{M}, m_cp_len{cp_len}, m_taper_len{taper_len} {

    LOG_PHY_INFO("PhyFrameGen::PhyFrameGen(M,cp,taper) called {} {} {} ", M, cp_len, taper_len );

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
    m_ifft = FFT_CREATE_PLAN(m_M, m_x, m_X, FFT_DIR_BACKWARD, FFT_METHOD);

    m_frame_len = m_M + m_cp_len;    // frame length
    m_buf_tx = (liquid_float_complex*) FFT_MALLOC((m_frame_len)*sizeof(liquid_float_complex));


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

    // create tapering window and transition buffer
    m_taper   = (float*)                malloc(m_taper_len * sizeof(float));
    m_postfix = (liquid_float_complex*) malloc(m_taper_len * sizeof(liquid_float_complex));




    // initalize m_taper and m_postfix
    unsigned int i;

    for (i=0; i < m_taper_len; i++) {
        float t = ((float)i + 0.5f) / (float)(m_taper_len);
        float g = sinf(M_PI_2*t);
        m_taper[i] = g*g;
    }

    for (i=0; i < m_taper_len; i++)
        m_postfix[i] = 0.0f;

}

PhyFrameGen::~PhyFrameGen() {
    LOG_PHY_INFO("PhyFrameGen::~PhyFrameGen() called");
    // free up all stuff

}

/**
 * @brief initalize the STS seqence in frequency and time domain
 * 
 * @return int 
 */
int PhyFrameGen::init_STS() {

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
    // @todo check if we still do this in the final version - further the DC subcarriers should be checked again
    // @todo as the generated sequence is not 100% what the standard describes
    liquid_float_complex tmp;

    for (i=0; i<m_M2; i++) {
        tmp = m_STS[i];
        m_STS[i] = m_STS[i + m_M2];
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
        m_sts[i] *= g * 0.5;           // @todo CHECK CHECK how to handle the gain of the IQ signal ..seems to be overloading for testing lower

    return 0;
}



/**
 * @brief write_STS is creating the STS symbol in the time domain
 * 
 * @param _y 
 * @return int 
 */
int PhyFrameGen::write_STS(liquid_float_complex * _y)
{
    // unsigned int i;
    // unsigned int k;

    // for (i=0; i < (m_M + m_cp_len); i++) {
    //     k = (i + m_M - 2*m_cp_len) % m_M;
    //     _y[i] = m_sts[k];
    // }

    // // apply tapering window
    // for (i=0; i < m_taper_len; i++)
    //     _y[i] *= m_taper[i];

    memmove(m_x, m_sts, (m_M)*sizeof(liquid_float_complex));
    genSymbol(_y);
 
    return 0;
}






// generate symbol (add cyclic prefix/postfix, overlap)
//
//  ->|   |<- taper_len
//    +   +-----+-------------------+
//     \ /      |                   |
//      X       |      symbol       |
//     / \      |                   |
//    +---+-----+-------------------+----> time
//    |         |                   |
//    |<- cp  ->|<-       M       ->|
//
//  _q->x           :   input time-domain symbol [size: _q->M x 1]
//  _q->postfix     :   input:  post-fix from previous symbol [size: _q->taper_len x 1]
//                      output: post-fix from this new symbol
//  _q->taper       :   tapering window
//  _q->taper_len   :   tapering window length
//
//  _buffer         :   output sample buffer [size: (_q->M + _q->cp_len) x 1]
int PhyFrameGen::genSymbol(liquid_float_complex * _buffer)
{
    // copy input symbol with cyclic prefix to output symbol
    memmove( &_buffer[0],        &m_x[m_M-m_cp_len], m_cp_len * sizeof(liquid_float_complex));
    memmove( &_buffer[m_cp_len], &m_x[           0], m_M      * sizeof(liquid_float_complex));
    
    // apply tapering window to over-lapping regions
    unsigned int i;
    for (i=0; i<m_taper_len; i++) {
        _buffer[i] *= m_taper[i];
        _buffer[i] += m_postfix[i] * m_taper[m_taper_len-i-1];
    }

    // copy post-fix to output (first 'taper_len' samples of input symbol)
    memmove(m_postfix, m_x, m_taper_len*sizeof(liquid_float_complex));


    // ensure that buffer is empty
    m_tx_buffer->data.clear();

    //move time iq data in tx symbol buffer
    for(auto s=0; s < (m_M+m_cp_len); s++) {
        m_tx_buffer->data.push_back(m_x[s]);    
    }

 //   std::cout << "gs" << std::endl;               // DEBUG

    return 0;
}






void PhyFrameGen::setTXBuffer(const RadioIQDataPtr& buffer) {
//    std::lock_guard < std::mutex > lock(m_queue_bindings_mutex);
    LOG_RADIO_DEBUG("PhyFrameGen::setTXBuffer()");
    m_tx_buffer = buffer;
}