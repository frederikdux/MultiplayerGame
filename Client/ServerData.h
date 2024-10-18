//
// Created by Frederik on 08.10.2024.
//

#ifndef SERVERDATA_H
#define SERVERDATA_H
#include <string>


class ServerData {
private:
    //id, ip, port, name
    int id;
    std::string ip;
    int port;
    std::string name;

public:
    ServerData(int id, const std::string &ip, int port, const std::string &name)
        : id(id),
          ip(ip),
          port(port),
          name(name) {
    }

    //getters
    [[nodiscard]] int getId() const {
        return id;
    }

    [[nodiscard]] const std::string &getIp() const {
        return ip;
    }

    [[nodiscard]] int getPort() const {
        return port;
    }

    [[nodiscard]] const std::string &getName() const {
        return name;
    }
};



#endif //SERVERDATA_H
