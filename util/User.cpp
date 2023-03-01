//
// Created by fabian on 18.02.23.
//

#include <algorithm>
#include "User.h"

const std::string &User::getUsername() const {
    return username;
}

void User::setUsername(const std::string &user) {
    User::username = user;
}

const std::string &User::getPasswordHash() const {
    return password_hash;
}

void User::setPasswordHash(const std::string &passwordHash) {
    password_hash = passwordHash;
}

const std::vector<std::string> &User::getPermissions() const {
    return permissions;
}

void User::setPermissions(const std::vector<std::string> &perm) {
    User::permissions = perm;
}

void User::addPermission(std::string & perm) {
    permissions.push_back(perm);
}

bool User::hasPermission(const std::string & permission) {
    return std::any_of(permissions.begin(), permissions.end(),
                       [permission](const std::string & v) {return v == permission;});
}
