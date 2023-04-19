#include "phy/Radio.h"
#include <thread>
#include <chrono>
#include <complex>


Radio::Radio() : m_sampleBufferCnt{DEFAULT_SAMPLEBUFFERCNT} {
    LOG_RADIO_DEBUG("Radio::Radio() constructor");
    m_isRxTxRunning.store(false);
    m_isRX.store(true);                     // default is to run in RX mode

    // m_rx_buffer = std::make_shared<RadioIQData>();

    

}


Radio::Radio(int sampleBufferCnt) : m_sampleBufferCnt{sampleBufferCnt} {
    LOG_RADIO_DEBUG("Radio::Radio(int) constructor");
    m_isRxTxRunning.store(false);
    m_isRX.store(true);                     // default is to run in RX mode

    // m_rx_buffer = std::make_shared<RadioIQData>();

    std::cout << m_rx_buffer << std::endl;
}

Radio::~Radio() {
    LOG_RADIO_DEBUG("Radio() de-constructor");
}

void Radio::setup() {
    // defined in radio specific class (e.g. LimeRadio)
}

void Radio::setFrequency(float_t frequency) {
    // defined in radio specific class (e.g. LimeRadio)
}

void Radio::setSamplingRate(float_t sampling_rate, size_t oversampling) {
    // defined in radio specific class (e.g. LimeRadio)
}

void Radio::set_HW_RX() {
    // defined in radio specific class (e.g. LimeRadio)
};

void Radio::set_HW_TX(TxMode m) {
    // defined in radio specific class (e.g. LimeRadio)
};

int Radio::send_IQ_data() {
    // defined in radio specific class (e.g. LimeRadio)
    return 0;
};

int Radio::receive_IQ_data() {
    // defined in radio specific class (e.g. LimeRadio)
    return 0;
};

int Radio::send_Tone() {
    // defined in radio specific class (e.g. LimeRadio)
    return 0;
};

uint64_t Radio::get_sample_timestamp() {
    // defined in radio specific class (e.g. LimeRadio)
    return 0;
};


void Radio::setRXBuffer(const RadioIQDataPtr& buffer) {
    std::lock_guard < std::mutex > lock(m_queue_bindings_mutex);
    LOG_RADIO_DEBUG("Radio::setRXBuffer()");
    m_rx_buffer = buffer;
}

RadioIQDataPtr Radio::getRXBuffer() {
    std::lock_guard < std::mutex > lock(m_queue_bindings_mutex);
    // LOG_RADIO_DEBUG("Radio::getRXBuffer() ");
    return m_rx_buffer;
}

void Radio::setTXBuffer(const RadioIQDataPtr& buffer) {
    std::lock_guard < std::mutex > lock(m_queue_bindings_mutex);
    LOG_RADIO_DEBUG("Radio::setTXBuffer()");
    m_tx_buffer = buffer;
}

RadioIQDataPtr Radio::getTXBuffer() {
    std::lock_guard < std::mutex > lock(m_queue_bindings_mutex);
    // LOG_RADIO_DEBUG("Radio::getTXBuffer() ");
    return m_tx_buffer;
}
