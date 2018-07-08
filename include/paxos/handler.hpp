#ifndef __HANDLER_HPP_INCLUDED__
#define __HANDLER_HPP_INCLUDED__

#include <string>
#include <vector>

#include "paxos/decree.hpp"
#include "paxos/replicaset.hpp"
#include "paxos/signal.hpp"


namespace paxos
{


//
// Handler that will be executed after a decree has been accepted. It
// is expected to be idempotent.
//
using Handler = std::function<void(std::string entry)>;


class DecreeHandler
{
public:

    virtual void operator()(std::string entry) = 0;
};


class EmptyDecreeHandler : public DecreeHandler
{
public:

    virtual void operator()(std::string entry) override;
};


class CompositeHandler : public DecreeHandler
{
public:

    CompositeHandler();

    CompositeHandler(Handler handler);

    virtual void operator()(std::string entry) override;

    void AddHandler(Handler handler);

private:

    std::vector<Handler> handlers;
};


class HandleAddReplica : public DecreeHandler
{
public:

    HandleAddReplica(
        std::string location,
        Replica legislator,
        std::shared_ptr<ReplicaSet>& legislators,
        std::shared_ptr<Signal> signal,
        std::function<void(
            std::shared_ptr<ReplicaSet>,
            std::ostream&)> save_replicaset=SaveReplicaSet);

    virtual void operator()(std::string entry) override;

private:

    std::string location;

    Replica legislator;

    std::shared_ptr<ReplicaSet>& legislators;

    std::shared_ptr<Signal> signal;

    std::function<void(
        std::shared_ptr<ReplicaSet>,
        std::ostream&)> save_replicaset;
};


class HandleRemoveReplica : public DecreeHandler
{
public:

    HandleRemoveReplica(
        std::string location,
        Replica legislator,
        std::shared_ptr<ReplicaSet>& legislators,
        std::shared_ptr<Signal> signal,
        std::function<void(
            std::shared_ptr<ReplicaSet>,
            std::ostream&)> save_replicaset=SaveReplicaSet);

    virtual void operator()(std::string entry) override;

private:

    std::string location;

    Replica legislator;

    std::shared_ptr<ReplicaSet>& legislators;

    std::shared_ptr<Signal> signal;

    std::function<void(
        std::shared_ptr<ReplicaSet>,
        std::ostream&)> save_replicaset;
};


}


#endif
