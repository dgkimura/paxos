#ifndef __PAXOS_HPP_INCLUDED__
#define __PAXOS_HPP_INCLUDED__


#include <memory>
#include <vector>

#include <acceptor.hpp>
#include <learner.hpp>
#include <proposer.hpp>
#include <receiver.hpp>
#include <sender.hpp>
#include <updater.hpp>


struct Instance
{
    std::shared_ptr<Receiver> Receiver;
    std::shared_ptr<Sender> Sender;
    std::shared_ptr<ProposerContext> Proposer;
    std::shared_ptr<AcceptorContext> Acceptor;
    std::shared_ptr<LearnerContext> Learner;
    std::shared_ptr<UpdaterContext> Updater;
};


std::vector<Instance *>_Instances;


Instance *CreateInstance();


void Init();


#endif
