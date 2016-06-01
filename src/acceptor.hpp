#ifndef __ACCEPTOR_HPP_INCLUDED__
#define __ACCEPTOR_HPP_INCLUDED__

#include <memory>

#include <callback.hpp>
#include <context.hpp>
#include <messages.hpp>
#include <receiver.hpp>
#include <sender.hpp>


struct AcceptorContext : public Context
{
    Decree promised_decree;
    Decree accepted_decree;
};


void RegisterAcceptor(
    std::shared_ptr<Receiver> receiver,
    std::shared_ptr<Sender> sender,
    std::shared_ptr<AcceptorContext> context);


void HandlePrepare(
    Message message,
    std::shared_ptr<AcceptorContext> context,
    std::shared_ptr<Sender> sender);


void HandleAccept(
    Message message,
    std::shared_ptr<AcceptorContext> context,
    std::shared_ptr<Sender> sender);


#endif
