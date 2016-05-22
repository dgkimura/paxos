#include <learner.hpp>


void
RegisterLearner(Receiver receiver, std::shared_ptr<Sender> sender)
{
    using namespace std::placeholders;

    std::shared_ptr<LearnerContext> context(new LearnerContext());

    receiver.RegisterCallback<AcceptedMessage>(
        Callback(std::bind(HandleProclaim, _1, context, sender))
    );
}


void
HandleProclaim(Message message, std::shared_ptr<LearnerContext> context, std::shared_ptr<Sender> sender)
{
    if (context->accepted_map.find(message.decree) == context->accepted_map.end())
    {
        context->accepted_map[message.decree] = std::shared_ptr<ReplicaSet>(new ReplicaSet());
    }
    if (context->full_replicaset.Contains(message.from))
    {
        context->accepted_map[message.decree]->Add(message.from);
    }

    int minimum_quorum = context->full_replicaset.GetSize() / 2 + 1;
    int accepted_quorum = context->accepted_map[message.decree]->GetSize();

    if (accepted_quorum >= minimum_quorum)
    {
        context->ledger->Append(message.decree);
    }
}
