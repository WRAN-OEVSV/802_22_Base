
#pragma once
#ifndef WRAN_ARGON2WRAPPER_H
#define WRAN_ARGON2WRAPPER_H

#include <string>

class Argon2Wrapper {
private:
    uint32_t t_cost;
    uint32_t m_cost;
    uint32_t parallelism;
public:
    Argon2Wrapper();
    Argon2Wrapper(uint32_t iterations, uint32_t memory, uint32_t parallelism);
    bool verifyHash(std::string password, std::string hash);
    std::string generateHash(std::string password);
};


#endif //WRAN_ARGON2WRAPPER_H
