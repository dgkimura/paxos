#include <updater.hpp>


void
RegisterUpdater(
    std::shared_ptr<Receiver> receiver,
    std::shared_ptr<Sender> sender,
    std::shared_ptr<UpdaterContext> context)
{
    using namespace std::placeholders;

    receiver->RegisterCallback(
        Callback(std::bind(HandleUpdate, _1, context, sender)),
        MessageType::UpdateMessage
    );
}


void
HandleUpdate(
    Message message,
    std::shared_ptr<UpdaterContext> context,
    std::shared_ptr<Sender> sender)
{
}
