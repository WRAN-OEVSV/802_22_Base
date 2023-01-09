# Repo for WRAN project 802.22 Base Code

WRAN project 802.22 base code for SDR, PHY, MAC


## 2023-01-09

code is work-in progress currently on the SDR IQ receiving and transmitting

IQ data is currently verfied by using GnuRadio (as TX) and Gnu Octave to check the data

next step is to create a basic PHY thread which is able to transmit and receive superframes/frames with STS and LTS to sync and exact frame timing (using IQ timestamping of the Lime)

some basic stuff gets added along the way (e.g logging, basic system configfiles, ...)

CMake will create a radio_test binary which is receiving IQ data for 2sec and is writting it to a CSV file for futher checking with Octave; settings of freq, sampling, oversampling is in the SystemConfig.json file