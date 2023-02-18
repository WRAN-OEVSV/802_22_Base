//
// Created by fabian on 18.02.23.
//

#ifndef WRAN_USER_H
#define WRAN_USER_H


#include <string>
#include <vector>

class User {
private:
    std::string username;
    std::string password_hash;
    std::vector<std::string> permissions;
public:
    [[nodiscard]] const std::string &getUsername() const;

    void setUsername(const std::string &username);

    [[nodiscard]] const std::string &getPasswordHash() const;

    void setPasswordHash(const std::string &passwordHash);

    [[nodiscard]] const std::vector<std::string> &getPermissions() const;

    void setPermissions(const std::vector<std::string> &permissions);

    void addPermission(std::string &perm);

    bool hasPermission(const std::string & permission);
};


#endif //WRAN_USER_H
