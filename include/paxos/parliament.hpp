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


namespace paxos
{


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
               Handler accept_handler=[](std::string entry){});

    Parliament(Replica legislator,
               std::shared_ptr<ReplicaSet> legislators,
               std::shared_ptr<Ledger> ledger,
               std::shared_ptr<Receiver> receiver,
               std::shared_ptr<Sender> sender,
               std::shared_ptr<AcceptorContext> acceptor,
               std::shared_ptr<ProposerContext> proposer,
               std::shared_ptr<LearnerContext> learner);

    bool AddLegislator(std::string address,
                       short port,
                       std::string remote=".");

    bool RemoveLegislator(std::string address,
                          short port,
                          std::string remote=".");

    std::shared_ptr<ReplicaSet> GetLegislators();

    void SendProposal(std::string entry);

    void SetActive();

    void SetInactive();

    AbsenteeBallots GetAbsenteeBallots(int max_ballots);

private:

    Replica legislator;

    std::shared_ptr<ReplicaSet> legislators;

    std::shared_ptr<Receiver> receiver;

    std::shared_ptr<Sender> sender;

    std::shared_ptr<Listener> bootstrap;

    std::shared_ptr<Ledger> ledger;

    std::shared_ptr<LearnerContext> learner;

    std::string location;

    std::shared_ptr<Signal> signal;

    void hookup_legislator(Replica replica,
                           std::shared_ptr<ProposerContext> proposer,
                           std::shared_ptr<AcceptorContext> acceptor);

    void send_decree(Decree decree);
};


}


#endif
