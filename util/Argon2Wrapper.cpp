//
// Created by fabian on 12.02.23.
//
#define HASHLEN 32
#define ENCODEDLEN 150
#define SALTLEN 16
#include "argon2.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <random>

#include "Argon2Wrapper.h"

Argon2Wrapper::Argon2Wrapper() : t_cost{ 2 }, m_cost {1<<16}, parallelism {1}  {
}

Argon2Wrapper::Argon2Wrapper(uint32_t iterations, uint32_t memory, uint32_t parallelism) : t_cost{ iterations }, m_cost {memory}, parallelism {parallelism}  {
}

bool Argon2Wrapper::verifyHash(std::string password, std::string hash) {
    if (hash.rfind("$argon2id") == 0) {
        return argon2id_verify(hash.c_str(), password.c_str(), password.size()) == ARGON2_OK;
    } else if (hash.rfind("$argon2i") == 0) {
        return argon2i_verify(hash.c_str(), password.c_str(), password.size()) == ARGON2_OK;
    } else if (hash.rfind("$argon2d") == 0) {
        return argon2d_verify(hash.c_str(), password.c_str(), password.size()) == ARGON2_OK;
    } else {
        return false; // the hash is not valid
    }
}

std::string Argon2Wrapper::generateHash(std::string password) {

    char encoded_hash[ENCODEDLEN];

    uint8_t salt[SALTLEN];
    // create a random salt
    std::random_device random_dev;
    for (unsigned char & i : salt) {
        i = random_dev();
    }

    auto *pwd = strdup(password.c_str());
    uint32_t pwd_len = strlen(pwd);

    argon2d_hash_encoded(t_cost, m_cost, parallelism, pwd, pwd_len, salt, SALTLEN, HASHLEN, encoded_hash, ENCODEDLEN);
    memset(pwd, 0, pwd_len);
    free(pwd);
    return encoded_hash;
}
