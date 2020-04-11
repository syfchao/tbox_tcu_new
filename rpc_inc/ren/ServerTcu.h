//
// Created by renhono on 2020/3/19.
//

#ifndef RPCSERVER__SERVERTCU_H_
#define RPCSERVER__SERVERTCU_H_
#include <iostream>
#include "server.h"
#include "wifiutil.h"

class ServerTcu : public rpc::server {
 public:
  ServerTcu(uint16_t port);
  ~ServerTcu();
  void TcuInterfaceInit();
 private:
  wifiUtil *m_wifiutill;

};

#endif //RPCSERVER__SERVERTCU_H_
