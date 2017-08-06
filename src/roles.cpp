#include "paxos/logging.hpp"
#include "paxos/roles.hpp"


void
RegisterProposer(
    std::shared_ptr<Receiver> receiver,
    std::shared_ptr<Sender> sender,
    std::shared_ptr<ProposerContext> context)
{
    using namespace std::placeholders;

    receiver->RegisterCallback(
        Callback(std::bind(HandleRequest, std::placeholders::_1, context, sender)),
        MessageType::RequestMessage
    );
    receiver->RegisterCallback(
        Callback(std::bind(HandlePromise, std::placeholders::_1, context, sender)),
        MessageType::PromiseMessage
    );
    receiver->RegisterCallback(
        Callback(std::bind(HandleNackTie, std::placeholders::_1, context, sender)),
        MessageType::NackTieMessage
    );
    receiver->RegisterCallback(
        Callback(std::bind(HandleNack, std::placeholders::_1, context, sender)),
        MessageType::NackMessage
    );
    receiver->RegisterCallback(
        Callback(std::bind(HandleResume, std::placeholders::_1, context, sender)),
        MessageType::ResumeMessage
    );
}


void
RegisterAcceptor(
    std::shared_ptr<Receiver> receiver,
    std::shared_ptr<Sender> sender,
    std::shared_ptr<AcceptorContext> context)
{
    using namespace std::placeholders;

    receiver->RegisterCallback(
        Callback(std::bind(HandlePrepare, std::placeholders::_1, context, sender)),
        MessageType::PrepareMessage
    );
    receiver->RegisterCallback(
        Callback(std::bind(HandleAccept, std::placeholders::_1, context, sender)),
        MessageType::AcceptMessage
    );
}


void
RegisterLearner(
    std::shared_ptr<Receiver> receiver,
    std::shared_ptr<Sender> sender,
    std::shared_ptr<LearnerContext> context)
{
    using namespace std::placeholders;

    receiver->RegisterCallback(
        Callback(std::bind(HandleAccepted, std::placeholders::_1, context, sender)),
        MessageType::AcceptedMessage
    );
    receiver->RegisterCallback(
        Callback(std::bind(HandleUpdated, std::placeholders::_1, context, sender)),
        MessageType::UpdatedMessage
    );
}


void
RegisterUpdater(
    std::shared_ptr<Receiver> receiver,
    std::shared_ptr<Sender> sender,
    std::shared_ptr<UpdaterContext> context)
{
    using namespace std::placeholders;

    receiver->RegisterCallback(
        Callback(std::bind(HandleUpdate, std::placeholders::_1, context, sender)),
        MessageType::UpdateMessage
    );
}


void
HandleRequest(
    Message message,
    std::shared_ptr<ProposerContext> context,
    std::shared_ptr<Sender> sender)
{
    LOG(LogLevel::Info) << "HandleRequest | " << message.decree.number << "|"
                        << Serialize(message);
    if (!message.decree.content.empty())
    {
        context->requested_values.push_back(
            std::make_tuple(message.decree.content, message.decree.type));
    }

    Message response = Response(message, MessageType::PrepareMessage);
    if (!context->ledger->IsEmpty())
    {
        response.decree.number = context->ledger->Tail().number + 1;
    }
    else
    {
        response.decree.number = 1;
    }
    response.decree.author = message.to;
    response.decree.root_number = response.decree.number;
    response.decree.content = "";

    sender->ReplyAll(response);
}


void
HandlePromise(
    Message message,
    std::shared_ptr<ProposerContext> context,
    std::shared_ptr<Sender> sender)
{
    LOG(LogLevel::Info) << "HandlePromise | " << message.decree.number << "|"
                        << Serialize(message);

    if (IsDecreeHigher(message.decree,
                       context->highest_proposed_decree.Value()) &&
        context->replicaset->Contains(message.from))
    {
        //
        // If the messaged decree is higher than any previoiusly promised
        // decree and the sender is a known sender in our replica set then
        // remember the sent decree as the highest promised decree.
        //
        context->highest_proposed_decree = message.decree;
    }

    if (context->promise_map.find(message.decree) == context->promise_map.end())
    {
        //
        // If there is no entry for the messaged decree then make an entry.
        //
        context->promise_map[message.decree] = std::make_shared<ReplicaSet>();
    }

    if (IsDecreeEqual(message.decree, context->highest_proposed_decree.Value()))
    {
        bool duplicate = context->promise_map[message.decree]
                                ->Contains(message.from);
        //
        // If the messaged decree is the highest promised decree then update
        // our promised decree map and calculate if majority of replicas have
        // sent promises for the decree.
        //
        context->promise_map[message.decree]->Add(message.from);

        int minimum_quorum = context->replicaset->GetSize() / 2 + 1;
        int received_promises = context->promise_map[message.decree]
                                       ->Intersection(context->replicaset)
                                       ->GetSize();

        //
        // If the messaged decree is a duplicate message allow a possible
        // resend of accept message.
        //
        if (received_promises == minimum_quorum ||
            (received_promises >= minimum_quorum && duplicate))
        {
            //
            // Acquire mutex to protect check and erase on requested_values.
            //
            std::lock_guard<std::mutex> lock(context->mutex);

            if (context->highest_proposed_decree.Value().content.empty() &&
                !context->requested_values.empty())
            {
                //
                // If the following are true...
                //
                //     i)   the majority of replicas have sent promises
                //     ii)  the highest proposed decree is empty
                //     iii) we have pending requested values
                //
                // ... then we should update the highest proposed decree with
                // the requested values.
                //
                Decree updated_highest_proposed_decree =
                    context->highest_proposed_decree.Value();
                updated_highest_proposed_decree.content = std::get<0>(
                    context->requested_values[0]);
                updated_highest_proposed_decree.type = std::get<1>(
                    context->requested_values[0]);

                context->highest_proposed_decree = updated_highest_proposed_decree;
                context->requested_values.erase(
                    context->requested_values.begin());
            }
            message.decree = context->highest_proposed_decree.Value();

            if (!message.decree.content.empty())
            {
                sender->ReplyAll(Response(message, MessageType::AcceptMessage));
            }
        }
    }

}


void
HandleNackTie(
    Message message,
    std::shared_ptr<ProposerContext> context,
    std::shared_ptr<Sender> sender)
{
    LOG(LogLevel::Info) << "HandleNackTie | " << message.decree.number << "|"
                        << Serialize(message);

    if (IsDecreeEqual(message.decree, context->ledger->Tail()))
    {
        //
        // Iff the current proposed decree is tied then retry with a higher
        // decree.
        //
        Message nack_response(
            message.decree,
            message.to,
            message.to,
            MessageType::PrepareMessage
        );
        nack_response.decree.number += 1;

        context->pause->Start();
        sender->ReplyAll(nack_response);
    }
}


void
HandleNack(
    Message message,
    std::shared_ptr<ProposerContext> context,
    std::shared_ptr<Sender> sender)
{
    LOG(LogLevel::Info) << "HandleNack    | " << message.decree.number << "|"
                        << Serialize(message);

    //
    // Acquire mutex to prevent race-conditions relating to compare and set of
    // the highest_nacked_decree.
    //
    std::lock_guard<std::mutex> lock(context->mutex);

    if (IsDecreeHigher(message.decree, context->highest_nacked_decree) &&
        message.decree.type == DecreeType::UserDecree &&
        !context->requested_values.empty())
    {
        context->ignore_handler(std::get<0>(context->requested_values.front()));
        context->highest_nacked_decree = message.decree;
        context->requested_values.erase(context->requested_values.begin());
    } else if (message.decree.type == DecreeType::AddReplicaDecree ||
               message.decree.type == DecreeType::RemoveReplicaDecree)
    {
        //
        // Upon Nack of add replica or remove replica, we must send signal to
        // unblock waiting thread.
        //
        context->signal->Set(false);
    }
}


void
HandleResume(
    Message message,
    std::shared_ptr<ProposerContext> context,
    std::shared_ptr<Sender> sender)
{
    LOG(LogLevel::Info) << "HandleResume  | " << message.decree.number << "|"
                        << Serialize(message);

    if (context->promise_map.find(message.decree) != context->promise_map.end()
        && context->promise_map[message.decree]
                  ->Intersection(context->replicaset)
                  ->GetSize() >= context->replicaset->GetSize())
    {
        //
        // If we have received an accepted message from every replica in the
        // replicaset for the given decree, then we are unlikely to receive
        // them again. Therefore we will reclaim memory.
        //
        context->promise_map.erase(message.decree);
    }

    if (IsDecreeIdentical(context->ledger->Tail(), message.decree))
    {
        if (context->requested_values.size() > 0)
        {
            //
            // Setup next round for pending proposals.
            //
            sender->Reply(
                Message(
                    Decree(),
                    message.to,
                    message.to,
                    MessageType::RequestMessage
                )
            );
        }
    }
}


void
HandlePrepare(
    Message message,
    std::shared_ptr<AcceptorContext> context,
    std::shared_ptr<Sender> sender)
{
    LOG(LogLevel::Info) << "HandlePrepare | " << message.decree.number << "|"
                        << Serialize(message);

    if (IsDecreeHigher(message.decree, context->promised_decree.Value()) ||
        IsDecreeIdentical(message.decree, context->promised_decree.Value()))
    {
        //
        // If the messaged decree is higher than any decree seen before or the
        // messaged decree is identical to the current promised decree then
        // save it on persistent storage and send a promised message.
        //
        context->promised_decree = message.decree;
        sender->Reply(Response(message, MessageType::PromiseMessage));
    }
    else if (IsDecreeEqual(message.decree, context->promised_decree.Value()))

    {
        //
        // If the messaged decree is equal to current promised decree and
        // sent from a different replica then there may be dueling
        // proposers. Send a NackTie and let the proposer handle it.
        //
        sender->Reply(Response(message, MessageType::NackTieMessage));
    }
    else
    {
        //
        // If the messaged decree is lower than current promised decree then.
        // the other proposer is behind. Send a Nack.
        //
        sender->Reply(Response(message, MessageType::NackMessage));
    }
}


void
HandleAccept(
    Message message,
    std::shared_ptr<AcceptorContext> context,
    std::shared_ptr<Sender> sender)
{
    LOG(LogLevel::Info) << "HandleAccept  | " << message.decree.number << "|"
                        << Serialize(message);

    if (IsDecreeHigherOrEqual(message.decree, context->promised_decree.Value()))
    {
        if (IsDecreeHigher(message.decree, context->accepted_decree.Value()))
        {
            context->accepted_decree = message.decree;
            sender->ReplyAll(Response(message, MessageType::AcceptedMessage));
        }
    }
}


void
HandleAccepted(
    Message message,
    std::shared_ptr<LearnerContext> context,
    std::shared_ptr<Sender> sender)
{
    LOG(LogLevel::Info) << "HandleAccepted| " << message.decree.number << "|"
                        << Serialize(message);

    static std::mutex mutex;
    std::lock_guard<std::mutex> lock(mutex);

    if (context->accepted_map.find(message.decree) == context->accepted_map.end())
    {
        context->accepted_map[message.decree] = std::shared_ptr<ReplicaSet>(
                                                    new ReplicaSet());
    }
    if (context->replicaset->Contains(message.from))
    {
        context->accepted_map[message.decree]->Add(message.from);
    }

    int minimum_quorum = context->replicaset->GetSize() / 2 + 1;
    int accepted_quorum = context->accepted_map[message.decree]
                                 ->Intersection(context->replicaset)
                                 ->GetSize();

    if (accepted_quorum >= minimum_quorum)
    {
        if (IsRootDecreeOrdered(context->ledger->Tail(), message.decree)
            && !context->is_observer)
        {
            //
            // If decree is ordered then append to ledger. We check the decree
            // because decree may be incremented in response to NACK-tie.
            //
            context->ledger->Append(message.decree);

            if (accepted_quorum >= minimum_quorum )
            {
                //
                // A quorum has been accepted so we should resume to allow
                // ourselves to send proposals in the next election.
                //
                Message response;
                response.to = message.to;
                response.type = MessageType::ResumeMessage;
                response.decree = context->ledger->Tail();
                response.decree.content = "";
                sender->Reply(response);
            }
        }
        else if (IsDecreeLower(context->ledger->Tail(), message.decree))
        {
            //
            // Save the decree in memory if the decree is a future decree that
            // has not yet been written to our ledger.
            //
            context->tracked_future_decrees.push(message.decree);
        } else if (IsDecreeEqual(context->ledger->Tail(), message.decree))
        {
            //
            // Decree was already accepted so we should resume to allow
            // ourselves to send proposals in the next election.
            //
            Message response;
            response.to = message.to;
            response.type = MessageType::ResumeMessage;
            response.decree = context->ledger->Tail();
            response.decree.content = "";
            sender->Reply(response);
        }
        if (context->tracked_future_decrees.size() > 0 &&
            IsDecreeOrdered(context->ledger->Tail(),
                            context->tracked_future_decrees.top()) &&
            !context->is_observer)
        {
            Decree current_decree, previous_decree = context->ledger->Tail();
            while (context->tracked_future_decrees.size() > 0)
            {
                current_decree = context->tracked_future_decrees.top();
                if (IsDecreeOrdered(previous_decree, current_decree))
                {
                    //
                    // If tracked_future_decrees contains the next ordered
                    // decree then append to the ledger.
                    //
                    context->ledger->Append(current_decree);
                    previous_decree = current_decree;
                    context->tracked_future_decrees.pop();
                }
                else
                {
                    //
                    // Else tracked_future_decrees doesn't contain any more
                    // decrees of interest.
                    //
                    break;
                }
            }
        }
        else if (context->tracked_future_decrees.size() > 0 &&
                 !IsDecreeOrdered(context->ledger->Tail(),
                                  context->tracked_future_decrees.top()))
        {
            //
            // If the decree is not in order with the last decree recorded in
            // our ledger then there must be holes in our ledger.
            //
            Message response = Response(message, MessageType::UpdateMessage);
            response.decree = context->ledger->Tail();
            response.to = message.decree.author;
            sender->Reply(response);
        }
    }

    if (accepted_quorum == context->replicaset->GetSize())
    {
        //
        // All votes for decree have been accounted for. Now clean up memory.
        //
        std::lock_guard<std::mutex> lock(context->mutex);
        context->accepted_map.erase(message.decree);
    }
}


void
HandleUpdated(
    Message message,
    std::shared_ptr<LearnerContext> context,
    std::shared_ptr<Sender> sender)
{
    LOG(LogLevel::Info) << "HandleUpdated | " << message.decree.number << "|"
                        << Serialize(message);

    if (IsDecreeOrdered(context->ledger->Tail(), message.decree))
    {
        //
        // Append the decree iff the decree is in order with the last decree
        // recorded in our ledger.
        //
        context->ledger->Append(message.decree);

        Decree current_decree, previous_decree = message.decree;
        while (context->tracked_future_decrees.size() > 0)
        {
            current_decree = context->tracked_future_decrees.top();

            if (IsDecreeOrdered(previous_decree, current_decree))
            {
                //
                // If tracked_future_decrees contains the next ordered decree
                // then append to the ledger.
                //
                context->ledger->Append(current_decree);
                previous_decree = current_decree;
                context->tracked_future_decrees.pop();
            }
            else
            {
                //
                // Else tracked_future_decrees doesn't contain any more decrees
                // of interest.
                //
                break;
            }
        }

        if (IsDecreeEqual(message.decree, previous_decree))
        {
            //
            // If the tracked_future_decrees did not contain the next ordered
            // decree then ask the message sender to send us more updates.
            //
            sender->Reply(Response(message, MessageType::UpdateMessage));
        }
    }
    else if (message.decree.number == 0)
    {
        //
        // Zero decree means that we are now up to date so we should continue.
        //
        Message response;
        response.to = message.to;
        response.type = MessageType::ResumeMessage;
        response.decree = context->ledger->Tail();
        response.decree.content = "";
        sender->Reply(response);
    }
}


void
HandleUpdate(
    Message message,
    std::shared_ptr<UpdaterContext> context,
    std::shared_ptr<Sender> sender)
{
    LOG(LogLevel::Info) << "HandleUpdate| " << message.decree.number << "|"
                        << Serialize(message);

    //
    // Get the next logical ordered decree in our ledger. If we do not have the
    // next logical decree then we get the zero decree.
    //
    message.decree = context->ledger->Next(message.decree);
    sender->Reply(Response(message, MessageType::UpdatedMessage));
}
