#include <memory>

#include <callback.hpp>
#include <context.hpp>
#include <messages.hpp>
#include <receiver.hpp>
#include <sender.hpp>


void RegisterProposer(Receiver receiver, Sender sender);

void HandleRequest(Message message, std::shared_ptr<Context> context, Sender& sender);

void HandlePromise(Message message, std::shared_ptr<Context> context, Sender& sender);

void HandleAccepted(Message message, std::shared_ptr<Context> context, Sender& sender);
