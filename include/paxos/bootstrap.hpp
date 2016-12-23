#ifndef __BOOTSTRAP_HPP_INCLUDED__
#define __BOOTSTRAP_HPP_INCLUDED__

#include "paxos/replicaset.hpp"


class BootstrapListener
{
public:

    BootstrapListener(std::string address, short port);
};


void SendBootstrap(Replica replica, std::string location);


#endif
