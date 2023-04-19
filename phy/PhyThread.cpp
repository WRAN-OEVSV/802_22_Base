
#include "phy/PhyThread.h"


PhyThread::PhyThread(PhyMode mode) : m_phyMode{mode}, m_currentSampleTimestamp{0} {
    LOG_PHY_INFO("PhyThread::PhyThread(mode) constructor - phy is runing in mode {}", (int) mode);

    PhyThread(mode, DEFAULT_SAMPLE_RATE, DEFAULT_OVERSAMPLING, DEFAULT_CENTER_FREQ);

}

PhyThread::PhyThread(PhyMode mode, float_t samp_rate, size_t oversampling, float_t center_freq) 
                : m_phyMode{mode}, m_currentSampleTimestamp{0}, m_samp_rate{samp_rate}, m_oversampling{oversampling}, m_center_freq{center_freq} {
    LOG_PHY_INFO("PhyThread::PhyThread() constructor - phy is runing in mode {}", (int) mode);
    terminated.store(false);
    stopping.store(false);

    m_frameSync = PhyFrameSync(PHY_SUBCARRIERS__M, PHY_CP_STS_LTS, PHY_TAPERLEN);

    m_frameGen = PhyFrameGen(PHY_SUBCARRIERS__M, PHY_CP_STS_LTS, PHY_TAPERLEN);
    m_frameGen.setTXBuffer(m_iqbuffer_tx);

    // @todo change this to modular approach to being able to select what hardware gets initalized
    m_sdrRadio = new LimeRadio(3200);                   // @todo change this 3200 is only for testing purpose at the moment
    m_sdrRadio->setFrequency(m_center_freq);
    m_sdrRadio->setSamplingRate(m_samp_rate, m_oversampling);
    m_sdrRadio->setRXBuffer(m_iqbuffer_rx);
    m_sdrRadio->setTXBuffer(m_iqbuffer_tx);

    m_framestart_timestamp = 0;

}

PhyThread::~PhyThread() {
    LOG_PHY_INFO("PhyThread::~PhyThread() de-constructor");
    terminated.store(true);
    stopping.store(true);

    delete(m_sdrRadio);
}

void PhyThread::threadMain() {
    terminated.store(false);
    stopping.store(false);
    try {
        run();
    }
    catch (...) {
        terminated.store(true);
        stopping.store(true);
        throw;
    }
  
    terminated.store(true);
    stopping.store(true);
}

void PhyThread::terminate() {
    stopping.store(true);
}



void PhyThread::run() {
    // do signal processing magic here

    LOG_PHY_INFO("PHY thread starting.");


    // m_IQdataRXQueue = std::static_pointer_cast<RadioThreadIQDataQueue>( PhyThread::getRXQueue());
    // m_IQdataTXQueue = std::static_pointer_cast<RadioThreadIQDataQueue>( PhyThread::getTXQueue());


    // ** IMPORTANT INFORMATION **
    // depending on the state and mode the handling of IQ data is either single sample or whole symbol oriented

    auto debug_counter = 0;

    // what mode is the PHY running
    switch (m_phyMode)
    {
    case BASESTATION:
        LOG_PHY_INFO("PHY is running as BaseStation");

        m_isPhyRunning.store(true);


        // @todo
        // first basic goal is to create the LTS/STS every 10ms w/o any data header,...
        // next step would then be to create a header symbol which is sent by the basestation and received correctly by the CPE

        m_framestart_timestamp = m_sdrRadio->get_sample_timestamp() + 2000;

        // test set TX
        m_sdrRadio->set_HW_TX(Radio::TxMode::TX_6M);


        while(!stopping)
        {
            m_frameGen.create_STS_symbol();

//            std::cout << m_iqbuffer_tx->data.size() << std::endl;

            // for(auto x: m_iqbuffer_tx->data) {
            //     std::cout << x.real() << " : " << x.imag() << std::endl;
            // }

            m_iqbuffer_tx->timestampFirstSample = m_framestart_timestamp;
            
            m_sdrRadio->send_IQ_data();
//            m_sdrRadio->send_Tone();

            while(m_sdrRadio->get_sample_timestamp()+4100 < m_framestart_timestamp) {
                //std::cout << ".";
            }
//            std::cout << std::endl;

            m_framestart_timestamp += 22850;

            debug_counter += 1;

        }

        std::cout << "debug counter: " << debug_counter << std::endl;

        break;


    case CPE:
        LOG_PHY_INFO("PHY is running as CPE");

        m_isPhyRunning.store(true);

        // test set RX
        m_sdrRadio->set_HW_RX();

        while(!stopping)
        {
            //PhyThread statemachine


            // m_rxIQdataOut = std::make_shared<RadioThreadIQData>();

            // if(m_IQdataRXQueue->size() > 5) {
            //     m_IQdataRXQueue->pop(m_rxIQdataOut);

            //     m_currentSampleTimestamp = m_rxIQdataOut->timestampFirstSample;
            //     std::cout << m_currentSampleTimestamp << " " << m_rxIQdataOut->data.size() << std::endl;

            //     for(auto sample: m_rxIQdataOut->data)
            //     {
            //         m_frameSync.m_currentSampleTimestamp = m_currentSampleTimestamp;
            //         m_frameSync.execute(sample);
            //         m_currentSampleTimestamp++;
            //     }
            // }


            // test framesync

            // frame sync topics are sample oriented

            // read samples from sdr radio; amount of samples is defined when sdr class gets initalized
            m_sdrRadio->receive_IQ_data();

            m_currentSampleTimestamp = m_iqbuffer_rx->timestampFirstSample;

            std::cout << m_currentSampleTimestamp << " " << m_iqbuffer_rx->data.size() << std::endl; //debug

            for(auto sample: m_iqbuffer_rx->data)
            {
                m_frameSync.m_currentSampleTimestamp = m_currentSampleTimestamp;
                m_frameSync.execute(sample);
                m_currentSampleTimestamp++;
            }

        }


        break;


    case TEST:

        m_IQdataRXQueue = std::static_pointer_cast<RadioThreadIQDataQueue>( PhyThread::getRXQueue());
        m_IQdataTXQueue = std::static_pointer_cast<RadioThreadIQDataQueue>( PhyThread::getTXQueue());

        while(!stopping)
        {
            m_rxIQdataOut = std::make_shared<RadioThreadIQData>();

            if(m_IQdataRXQueue->size() > 5) {
                m_IQdataRXQueue->pop(m_rxIQdataOut);

                m_currentSampleTimestamp = m_rxIQdataOut->timestampFirstSample;
                std::cout << m_currentSampleTimestamp << " " << m_rxIQdataOut->data.size() << std::endl;

                for(auto sample: m_rxIQdataOut->data)
                {
                    m_frameSync.m_currentSampleTimestamp = m_currentSampleTimestamp;
                    m_frameSync.execute(sample);
                    m_currentSampleTimestamp++;
                }
            }
        }

        break;

    default:
        LOG_PHY_INFO("selected PHY mode is not supported - stopping");

        break;
    }

    m_isPhyRunning.store(false);

}




bool PhyThread::phyConfig(/*parameter*/) {

//  // validate input
//     if (_M < 8)
//         return liquid_error_config("ofdmflexframesync_create(), less than 8 subcarriers");
//     if (_M % 2)
//         return liquid_error_config("ofdmflexframesync_create(), number of subcarriers must be even");
//     if (_cp_len > _M)
//         return liquid_error_config("ofdmflexframesync_create(), cyclic prefix length cannot exceed number of subcarriers");

    return true;
}




// IQ Data handling Queue == Buffer  @todo adopt wording
// data buffer is handled via shared ptrs which are handed over to the SDR radio class

void PhyThread::setRXQueue(const ThreadIQDataQueueBasePtr& threadQueue) {
    std::lock_guard < std::mutex > lock(m_queue_bindings_mutex);
    LOG_PHY_DEBUG("PhyThread::setRXQueue()");
    m_rx_queue = threadQueue;
}

ThreadIQDataQueueBasePtr PhyThread::getRXQueue() {
    std::lock_guard < std::mutex > lock(m_queue_bindings_mutex);
    LOG_PHY_DEBUG("PhyThread::getRXQueue() ");
    return m_rx_queue;
}

void PhyThread::setTXQueue(const ThreadIQDataQueueBasePtr& threadQueue) {
    std::lock_guard < std::mutex > lock(m_queue_bindings_mutex);
    LOG_PHY_DEBUG("PhyThread::setTXQueue()");
    m_tx_queue = threadQueue;
}

ThreadIQDataQueueBasePtr PhyThread::getTXQueue() {
    std::lock_guard < std::mutex > lock(m_queue_bindings_mutex);
    LOG_PHY_DEBUG("PhyThread::getTXQueue() ");
    return m_tx_queue;
}
