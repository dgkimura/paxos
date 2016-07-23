#ifndef __PAXOS_HPP_INCLUDED__
#define __PAXOS_HPP_INCLUDED__


#include <memory>

#include <replicaset.hpp>
#include <roles.hpp>
#include <sender.hpp>


class Parliament
{
public:

    Parliament();

    void AddLegislator(Replica replica);

    void RemoveLegislator(Replica replica);

    void CreateProposal(std::string entry);

private:

    std::shared_ptr<ReplicaSet> legislators;

    std::shared_ptr<Receiver> receiver;

    std::shared_ptr<Sender> sender;

    std::shared_ptr<Ledger> ledger;
};


#endif
