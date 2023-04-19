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
#include "phy/Radio.h"
#include "util/log.h"


/**
 * @brief PhyFrameGen class holds components which are required to create symboles
 *        The sending of the symbols is then done by the PhyThread which is handling
 *        the statemachine for the PHY
 */

class PhyFrameGen {
public:

    PhyFrameGen();


    PhyFrameGen(unsigned int M,
                 unsigned int cp_len,
                 unsigned int taper_len);


    ~PhyFrameGen();


    void create_STS_symbol() { write_STS(m_buf_tx); }

    void setTXBuffer(const RadioIQDataPtr& buffer);

protected:

    RadioIQDataPtr m_tx_buffer;

private:

    // symbol information
    unsigned int m_M;                               // number of subcarriers
    unsigned int m_M2;                              // number of subcarriers div by 2
    unsigned int m_cp_len;                          // cyclic prefix length
    unsigned int m_frame_len;                       // frame length ( M+cp)

    unsigned char * m_subcarrier_allocation_STS;    // subcarrier allocation (null, pilot, data)
    unsigned char * m_subcarrier_allocation_LTS;    // subcarrier allocation (null, pilot, data)


    // tapering/trasition
    unsigned int m_taper_len; // number of samples in tapering window/overlap
    float * m_taper;          // tapering window
    liquid_float_complex *m_postfix; // overlapping symbol buffer


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
    FFT_PLAN m_ifft;           // ifft object
    
    liquid_float_complex *m_X;      // frequency-domain buffer
    liquid_float_complex *m_x;      // time-domain buffer
    windowcf m_input_buffer;  // input sequence buffer

    liquid_float_complex *m_buf_tx;      // frequency-domain TX buffer


    // STS and LTS sequences
    liquid_float_complex *m_STS;     // STS sequence (freq) //_S0
    liquid_float_complex *m_sts;     // STS sequence (time) //_s0
    liquid_float_complex *m_LTS;     // LTS sequence (freq)
    liquid_float_complex *m_lts;     // LTS sequence (time)




    int init_STS_sctype(); 
    int init_LTS_sctype();

    int validate_sctype_STS(); // ofdmframe_validate_sctype(q->p, q->M, &q->M_null, &q->M_pilot, &q->M_data)

    int init_STS(); //ofdmframe_init_S0(q->p, q->M, q->S0, q->s0, &q->M_S0);

    int write_STS(liquid_float_complex * _y);

    int genSymbol(liquid_float_complex * _buffer);


};