#pragma once

#include <complex>
#include <iostream>
#include <fstream>

#include "liquid.h"

#include "phy/RadioThread.h"
#include "util/log.h"

void write_csv_file(RadioThread* sdr, RadioThreadIQDataQueuePtr iqpipe_rx, std::string csv_file);