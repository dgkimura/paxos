#ifndef __PAXOS_HPP_INCLUDED__
#define __PAXOS_HPP_INCLUDED__

#include <functional>
#include <memory>

#include <replicaset.hpp>
#include <roles.hpp>
#include <sender.hpp>


using DecreeHandler = std::function<void(std::string entry)>;


class Parliament
{
public:

    Parliament(std::string location_=".",
               DecreeHandler decree_handler=[](std::string entry){});

    void AddLegislator(std::string address, short port);

    void SetLegislator(std::string address, short port);

    void RemoveLegislator(Replica replica);

    void CreateProposal(std::string entry);

private:

    std::shared_ptr<ReplicaSet> legislators;

    std::shared_ptr<Receiver> receiver;

    std::shared_ptr<Sender> sender;

    std::shared_ptr<Ledger> ledger;

    std::string location;
};


#endif
