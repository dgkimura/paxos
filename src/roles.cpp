#include "paxos/logging.hpp"
#include "paxos/roles.hpp"


namespace paxos
{


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
    receiver->RegisterCallback(
        Callback(std::bind(HandleCleanup, std::placeholders::_1, context, sender)),
        MessageType::ResumeMessage
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

    std::lock_guard<std::mutex> lock(context->mutex);

    if (!message.decree.content.empty())
    {
        context->requested_values.push_back(
            std::make_tuple(message.decree.content, message.decree.type));
    }

    Message response = Response(message, MessageType::PrepareMessage);
    if (!context->ledger->IsEmpty() ||
        context->highest_proposed_decree.Value().number != 0)
    {
        //
        // If ledger has an entry or we have previously proposed a decree then
        // the next decree we want to issue is the max between those sources.
        //
        response.decree.number = std::max(
            context->highest_proposed_decree.Value().number,
            context->ledger->Tail().number + 1);
        response.decree.root_number = std::max(
            context->highest_proposed_decree.Value().root_number,
            context->ledger->Tail().root_number + 1);
    }
    else
    {
        response.decree.number = 1;
        response.decree.root_number = 1;
    }
    response.decree.author = message.to;
    response.decree.content = "";

    if (context->requested_values.size() > 0)
    {
        sender->ReplyAll(response);
    }
}


void
HandlePromise(
    Message message,
    std::shared_ptr<ProposerContext> context,
    std::shared_ptr<Sender> sender)
{
    LOG(LogLevel::Info) << "HandlePromise | " << message.decree.number << "|"
                        << Serialize(message);

    std::lock_guard<std::mutex> lock(context->mutex);

    if (IsDecreeHigher(message.decree, context->highest_proposed_decree.Value()) &&
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

    if (!message.decree.content.empty())
    {
        //
        // Promise with non-empty decree contents suggests that decree was once
        // in accept state. Therefore we should send accept on this decree
        // again and let propogate through to accepted.
        //
        sender->ReplyAll(Response(message, MessageType::AcceptMessage));
    }
    else if (IsDecreeIdentical(message.decree, context->highest_proposed_decree.Value()))
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

    std::lock_guard<std::mutex> lock(context->mutex);

    auto tail_decree = context->ledger->Tail();
    if (context->ntie_map.find(message.decree) == context->ntie_map.end() &&
        IsRootDecreeHigher(message.decree, tail_decree) &&
        IsRootDecreeEqual(message.decree, context->highest_proposed_decree.Value()) &&
        context->nacktie_time + context->interval < std::chrono::high_resolution_clock::now())
    {
        context->ntie_map.insert(message.decree);
        context->nacktie_time = std::chrono::high_resolution_clock::now();

        Message nack_response(
            message.decree,
            message.to,
            message.to,
            MessageType::PrepareMessage
        );
        nack_response.decree.number += 1;
        context->highest_nacktie_decree = message.decree;

        //
        // We purposefully do not increment the highest_proposed_decree here in
        // order to prevent contention of competing decrees. If it were updated
        // here then every new request and nack tie response would create more
        // completing decree. So we instead update it in HandlePromise.
        //
        context->pause->Start([&nack_response, &sender, &context]()
        {
            auto next = nack_response.decree;

            //
            // Check again before sending because during the time that we
            // paused a higher conflicting nack tie may have come along or the
            // decree may have already passed for another replica.
            //
            if (context->highest_nacktie_decree.number + 1 == next.number &&
                context->ledger->Tail().root_number + 1 == next.root_number)
            {
                context->highest_proposed_decree = next;
                sender->ReplyAll(nack_response);
            }
        });
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

    std::lock_guard<std::mutex> lock(context->mutex);

    if (context->nprepare_map.find(message.decree) == context->nprepare_map.end())
    {
        context->nprepare_map[message.decree] = std::make_tuple(std::make_shared<ReplicaSet>(), false);
    }

    if (std::get<1>(context->nprepare_map[message.decree]) == false)
    {
        std::get<0>(context->nprepare_map[message.decree])->Add(message.from);;

        int minimum_quorum = context->replicaset->GetSize() / 2 + 1;
        int received_nprepare = std::get<0>(context->nprepare_map[message.decree])
                                       ->Intersection(context->replicaset)
                                       ->GetSize();

        if (received_nprepare >= minimum_quorum)
        {
            std::get<1>(context->nprepare_map[message.decree]) = true;
            context->requested_values.push_front(
                std::make_tuple(message.decree.content, message.decree.type));
        }
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

    std::lock_guard<std::mutex> lock(context->mutex);

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
        auto decree = context->highest_proposed_decree.Value();
        if (IsRootDecreeEqual(message.decree, decree) &&
            context->resume_map.find(message.decree) == context->resume_map.end() &&
            decree.content != message.decree.content &&
            !decree.content.empty() &&
            std::get<1>(context->nprepare_map[decree]) == false)
        {
            context->requested_values.push_front(
                std::make_tuple(decree.content, decree.type));
            context->resume_map.insert(message.decree);
            std::get<1>(context->nprepare_map[decree]) = true;
        }

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


void
HandlePrepare(
    Message message,
    std::shared_ptr<AcceptorContext> context,
    std::shared_ptr<Sender> sender)
{
    LOG(LogLevel::Info) << "HandlePrepare | " << message.decree.number << "|"
                        << Serialize(message);

    std::lock_guard<std::mutex> lock(context->mutex);

    if (!context->accepted_decree.Value().content.empty())
    {
        //
        // If there is a pending accepted decree then we must reply with a
        // promise to the previous accepted decree otherwise we risk losing the
        // decree.
        //
        auto response = Response(message, MessageType::PromiseMessage);
        response.decree = context->accepted_decree.Value();
        sender->Reply(response);
    } else if (
        IsDecreeHigher(message.decree, context->promised_decree.Value()) ||
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
    else if (IsReplicaEqual(message.decree.author,
                            context->promised_decree.Value().author))
    {
        //
        // Even though decree is lower, author is same as promised replica
        // so we can reply with a promise message of the higher promised
        // decree.
        //
        auto response = Response(message, MessageType::PromiseMessage);
        response.decree = context->promised_decree.Value();
        sender->Reply(response);
    }
    else if (
        IsRootDecreeEqual(message.decree, context->promised_decree.Value()))
    {
        //
        // If the messaged decree is equal to current promised decree and
        // sent from a different replica then there may be dueling
        // proposers. Send a NackTie and let the proposer handle it.
        //
        sender->Reply(Response(message, MessageType::NackTieMessage));
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

    std::lock_guard<std::mutex> lock(context->mutex);

    if (IsRootDecreeHigher(message.decree, context->promised_decree.Value()) ||
        IsDecreeIdentical(message.decree, context->accepted_decree.Value()) ||
        IsRootDecreeHigher(message.decree, context->accepted_decree.Value()) ||
        IsDecreeIdentical(message.decree, context->accepted_decree.Value()))
    {
        if (IsRootDecreeHigher(message.decree, context->accepted_decree.Value()))
        {
            //
            // If the messaged decree is greater than current accepted decree
            // we can update the accepted decree information and send the
            // accepted message.
            //
            context->accepted_time = std::chrono::high_resolution_clock::now();
            context->accepted_decree = message.decree;
            context->accepted_set.insert(message.decree);
            sender->ReplyAll(Response(message, MessageType::AcceptedMessage));
        }
        else if (context->accepted_time + context->interval <
                 std::chrono::high_resolution_clock::now() &&
                 IsRootDecreeEqual(message.decree,
                                   context->accepted_decree.Value()))
        {
            //
            // If the messaged decree is equivalent to than current accepted
            // decree then we throttle the sending of accepted message.
            //
            context->accepted_time = std::chrono::high_resolution_clock::now();
            sender->ReplyAll(Response(message, MessageType::AcceptedMessage));
        }
    }
    else if (!context->accepted_set.contains(message.decree))
    {
        //
        // If the message decree is not higher than the accepted decree then we
        // reject it.
        //
        sender->Reply(Response(message, MessageType::NackMessage));
    }
}


void
HandleCleanup(
    Message message,
    std::shared_ptr<AcceptorContext> context,
    std::shared_ptr<Sender> sender)
{
    LOG(LogLevel::Info) << "HandleCleanup | " << message.decree.number << "|"
                        << Serialize(message);

    std::lock_guard<std::mutex> lock(context->mutex);

    auto accepted_decree = context->accepted_decree.Value();
    if (IsRootDecreeHigherOrEqual(message.decree, accepted_decree))
    {
        //
        // We reset the accepted decree content to "" to signal to the prepare
        // handler that there is not at pending accept decree to flush.
        //
        Decree d = context->accepted_decree.Value();
        d.content = "";
        context->accepted_decree = d;
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

    std::lock_guard<std::mutex> lock(context->mutex);

    if (context->accepted_map.find(message.decree) == context->accepted_map.end())
    {
        context->accepted_map[message.decree] = std::make_shared<ReplicaSet>();
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
            sender->Reply(response);
        }
        if (context->tracked_future_decrees.size() > 0 &&
            IsRootDecreeOrdered(context->ledger->Tail(),
                            context->tracked_future_decrees.top()) &&
            !context->is_observer)
        {
            Decree current_decree, previous_decree = context->ledger->Tail();
            while (context->tracked_future_decrees.size() > 0)
            {
                current_decree = context->tracked_future_decrees.top();
                if (IsRootDecreeOrdered(previous_decree, current_decree))
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
                 !IsRootDecreeOrdered(context->ledger->Tail(),
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

    std::lock_guard<std::mutex> lock(context->mutex);

    if (IsRootDecreeOrdered(context->ledger->Tail(), message.decree))
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

            if (IsRootDecreeOrdered(previous_decree, current_decree))
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


}
