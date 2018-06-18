#ifndef __ROLES_HPP_INCLUDED__
#define __ROLES_HPP_INCLUDED__


#include <map>
#include <memory>

#include "paxos/callback.hpp"
#include "paxos/context.hpp"
#include "paxos/messages.hpp"
#include "paxos/receiver.hpp"
#include "paxos/sender.hpp"
#include "paxos/serialization.hpp"


namespace paxos
{


/*
 * Registrator functions will setup appropriate handler to the given sender and
 * receiver. After registering, the receiver will send forward all incoming
 * messages to the appropriate handler.
 */

void RegisterProposer(
    std::shared_ptr<Receiver> receiver,
    std::shared_ptr<Sender> sender,
    std::shared_ptr<ProposerContext> context);


void RegisterAcceptor(
    std::shared_ptr<Receiver> receiver,
    std::shared_ptr<Sender> sender,
    std::shared_ptr<AcceptorContext> context);


void RegisterLearner(
    std::shared_ptr<Receiver> receiver,
    std::shared_ptr<Sender> sender,
    std::shared_ptr<LearnerContext> context);


void RegisterUpdater(
    std::shared_ptr<Receiver> receiver,
    std::shared_ptr<Sender> sender,
    std::shared_ptr<UpdaterContext> context);


/*
 * Handlers are called after the message of interest arrives. Context is a way
 * to save state between calls to the handler.
 */

void HandleRequest(
    Message message,
    std::shared_ptr<ProposerContext> context,
    std::shared_ptr<Sender> sender);


void HandlePromise(
    Message message,
    std::shared_ptr<ProposerContext> context,
    std::shared_ptr<Sender> sender);


void HandleNackTie(
    Message message,
    std::shared_ptr<ProposerContext> context,
    std::shared_ptr<Sender> sender);


void HandleNack(
    Message message,
    std::shared_ptr<ProposerContext> context,
    std::shared_ptr<Sender> sender);


void HandleResume(
    Message message,
    std::shared_ptr<ProposerContext> context,
    std::shared_ptr<Sender> sender);


void HandlePrepare(
    Message message,
    std::shared_ptr<AcceptorContext> context,
    std::shared_ptr<Sender> sender);


void HandleAccept(
    Message message,
    std::shared_ptr<AcceptorContext> context,
    std::shared_ptr<Sender> sender);


void HandleCleanup(
    Message message,
    std::shared_ptr<AcceptorContext> context,
    std::shared_ptr<Sender> sender);


void HandleAccepted(
    Message message,
    std::shared_ptr<LearnerContext> context,
    std::shared_ptr<Sender> sender);


void HandleUpdated(
    Message message,
    std::shared_ptr<LearnerContext> context,
    std::shared_ptr<Sender> sender);


void HandleUpdate(
    Message message,
    std::shared_ptr<UpdaterContext> context,
    std::shared_ptr<Sender> sender);


}


#endif
