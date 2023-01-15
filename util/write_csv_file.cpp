

#include "util/write_csv_file.h"



void write_csv_file(RadioThread* sdr, RadioThreadIQDataQueuePtr iqpipe_rx, std::string csv_file) {
        
    std::ofstream myfile(csv_file);

    // error checking if file is there ... maybe a bit overkill for a test script :-)
    if(!myfile.is_open()) {
        LOG_TEST_ERROR("cannot open {}", csv_file);

        LOG_TEST_ERROR("terminate thread and flush data");
        sdr->terminate();
        while(!sdr->isTerminated(100)) {
            //flush the iq queue so that it does not overrun
            LOG_TEST_ERROR("wait to terminate");
        }
        LOG_TEST_ERROR("thread is terminated");
    } else {

        // get the received IQ data via the queue which is attached to the SDR thread

        RadioThreadIQDataPtr iq;
        
        LOG_TEST_INFO("get data from queue and write to /tmp/ramdisk/IQData.csv ");
        while(sdr->isRxTxRunning()) {

            // get data from queue
            while(iqpipe_rx->pop(iq)) {

                uint64_t t = iq->timestampFirstSample;

                for(auto s: iq->data)
                {
                    if(myfile.is_open())
                        myfile << s.real() << ", " << s.imag()<< ", " << t << std::endl;
                    t++;
                }

            }

        }
        myfile.close();
        LOG_TEST_INFO("done write to /tmp/ramdisk/IQData.csv ");
    }

}