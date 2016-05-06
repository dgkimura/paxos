#include <callback.hpp>
#include <context.hpp>
#include <messages.hpp>
#include <receiver.hpp>
#include <sender.hpp>


void RegisterLearner(Receiver receiver, Sender& sender);
void HandleProclaim(Message message, std::shared_ptr<Context> context, Sender& sender);
