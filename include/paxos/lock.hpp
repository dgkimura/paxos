#ifndef __LOCK_HPP_INCLUDED__
#define __LOCK_HPP_INCLUDED__

#include "paxos/replicaset.hpp"
#include "paxos/sender.hpp"
#include "paxos/signal.hpp"


class distributed_lock
{
public:

    distributed_lock(
        Replica replica,
        std::shared_ptr<Sender> sender,
        std::string location,
        std::string lockname);

    void lock();

    void unlock();

    bool is_locked();

    std::shared_ptr<Signal> signal;

private:

    std::shared_ptr<Sender> sender;

    Replica replica;

    std::string location;

    std::string lockname;
};


#endif
