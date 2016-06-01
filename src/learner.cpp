#include <learner.hpp>


void
RegisterLearner(
    std::shared_ptr<Receiver> receiver,
    std::shared_ptr<Sender> sender,
    std::shared_ptr<LearnerContext> context)
{
    using namespace std::placeholders;

    receiver->RegisterCallback(
        Callback(std::bind(HandleProclaim, _1, context, sender)),
        MessageType::AcceptedMessage
    );
    receiver->RegisterCallback(
        Callback(std::bind(HandleProclaim, _1, context, sender)),
        MessageType::UpdatedMessage
    );
}


void
HandleProclaim(
    Message message,
    std::shared_ptr<LearnerContext> context,
    std::shared_ptr<Sender> sender)
{
    if (context->accepted_map.find(message.decree) == context->accepted_map.end())
    {
        context->accepted_map[message.decree] = std::shared_ptr<ReplicaSet>(new ReplicaSet());
    }
    if (context->replicaset->Contains(message.from))
    {
        context->accepted_map[message.decree]->Add(message.from);
    }

    int minimum_quorum = context->replicaset->GetSize() / 2 + 1;
    int accepted_quorum = context->accepted_map[message.decree]->GetSize();

    if (accepted_quorum >= minimum_quorum)
    {
        context->ledger->Append(message.decree);
    }
}


void
HandleUpdateed(
    Message message,
    std::shared_ptr<LearnerContext> context,
    std::shared_ptr<Sender> sender)
{
}
