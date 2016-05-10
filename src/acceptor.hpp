#include <memory>

#include <callback.hpp>
#include <context.hpp>
#include <messages.hpp>
#include <receiver.hpp>
#include <sender.hpp>


void RegisterAcceptor(Receiver receiver, Sender& sender);

void HandlePrepare(Message message, std::shared_ptr<Context> context, Sender& sender);

void HandleAccept(Message message, std::shared_ptr<Context> context, Sender& sender);
