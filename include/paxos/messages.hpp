#ifndef __MESSAGES_HPP_INCLUDED__
#define __MESSAGES_HPP_INCLUDED__

#include "paxos/decree.hpp"
#include "paxos/replicaset.hpp"


enum class MessageType
{
    //
    // RequestMessage sent when there is a new proposal.
    //
    RequestMessage,

    //
    // RetryRequestMessage sent to retry a previous proposal.
    //
    RetryRequestMessage,

    //
    // PrepareMessage sent to signal beginning of a round of paxos.
    //
    PrepareMessage,

    //
    // RetryPrepareMessage sent to conditionally undo last promise.
    //
    RetryPrepareMessage,

    //
    // PromiseMessage sent to acknowledge a prepare.
    //
    PromiseMessage,

    //
    // NackTieMessage sent as a response to prepare indicating a tie.
    //
    NackTieMessage,

    //
    // NackMessage sent as a negative response to prepare.
    //
    NackMessage,

    //
    // AcceptMessage sent if quorum has elected a leader.
    //
    AcceptMessage,

    //
    // AcceptedMessage sent to continue to second phase of concensus.
    //
    AcceptedMessage,

    //
    // ResumeMessage sent to prepare for a new round of paxos.
    //
    ResumeMessage,

    //
    // UpdateMessage sent when replica realizes it is behind.
    //
    UpdateMessage,

    //
    // UpdatedMessage sent to update a replica that has fallen behind.
    //
    UpdatedMessage
};


struct Message
{
    Decree decree;
    Decree original_decree;
    Replica from;
    Replica to;
    MessageType type;

    Message()
        : decree(), original_decree(), from(), to(), type()
    {
    }

    Message(Decree d, Replica f, Replica t, MessageType mtype)
        : decree(d), original_decree(d), from(f), to(t), type(mtype)
    {
    }
};


Message Response(Message message, MessageType type);


#endif
