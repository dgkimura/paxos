#include <memory>

#include <callback.hpp>
#include <context.hpp>
#include <messages.hpp>
#include <receiver.hpp>
#include <sender.hpp>


void RegisterAcceptor(Receiver receiver, std::shared_ptr<Sender> sender);


struct AcceptorContext : public Context
{
    Decree promised_decree;
    Decree accepted_decree;
};


void HandlePrepare(Message message, std::shared_ptr<AcceptorContext> context, std::shared_ptr<Sender> sender);


void HandleAccept(Message message, std::shared_ptr<AcceptorContext> context, std::shared_ptr<Sender> sender);
