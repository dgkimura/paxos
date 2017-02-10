#ifndef __PAXOS_HPP_INCLUDED__
#define __PAXOS_HPP_INCLUDED__

#include <functional>
#include <map>
#include <memory>

#include <paxos/bootstrap.hpp>
#include <paxos/decree.hpp>
#include <paxos/replicaset.hpp>
#include <paxos/roles.hpp>
#include <paxos/sender.hpp>


//
// Map of a decree ballots.
//
using AbsenteeBallots = std::map<Decree, std::shared_ptr<ReplicaSet>,
                                 compare_decree>;


class Parliament
{
public:

    Parliament(Replica legislator,
               std::string location=".",
               DecreeHandler decree_handler=DecreeHandler());

    Parliament(Replica legislator,
               std::shared_ptr<ReplicaSet> legislators,
               std::shared_ptr<Ledger> ledger,
               std::shared_ptr<Receiver> receiver,
               std::shared_ptr<Sender> sender,
               std::shared_ptr<AcceptorContext> acceptor);

    void AddLegislator(std::string address, short port, std::string location=".");

    void RemoveLegislator(std::string address, short port, std::string location=".");

    void SendProposal(std::string entry);

    void SetActive();

    void SetInactive();

    AbsenteeBallots GetAbsenteeBallots(int max_ballots);

private:

    std::shared_ptr<ReplicaSet> legislators;

    std::shared_ptr<Receiver> receiver;

    std::shared_ptr<Sender> sender;

    std::shared_ptr<Listener> bootstrap;

    std::shared_ptr<Ledger> ledger;

    std::shared_ptr<LearnerContext> learner;

    void hookup_legislator(Replica replica,
                           std::string location,
                           std::shared_ptr<AcceptorContext> acceptor);

    void send_decree(Decree decree);
};


#endif
