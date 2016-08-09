#ifndef __MESSAGES_HPP_INCLUDED__
#define __MESSAGES_HPP_INCLUDED__

#include <decree.hpp>
#include <replicaset.hpp>


enum class MessageType
{
    //
    // RequestMessage sent when there is a new proposal.
    //
    RequestMessage,

    //
    // PrepareMessage sent to signal beginning of a round of paxos.
    //
    PrepareMessage,

    //
    // PromiseMessage sent to acknowledge a prepare.
    //
    PromiseMessage,

    //
    // AcceptMessage sent if quorum has elected a leader.
    //
    AcceptMessage,

    //
    // AcceptedMessage sent to prepare for a new round of paxos.
    //
    AcceptedMessage,

    //
    // UpdateMessage sent when replica realizes it is behind.
    //
    UpdateMessage,

    //
    // UpdatedMessage sent to update a replica that has fallen behind.
    //
    UpdatedMessage
};


struct MessageTypeHash
{
    template <typename T>
    std::size_t operator()(T t) const
    {
        return static_cast<std::size_t>(t);
    }
};


struct Message
{
    Decree decree;
    Replica from;
    Replica to;
    MessageType type;

    Message()
        : decree(), from(), to(), type()
    {
    }

    Message(Decree d, Replica f, Replica t, MessageType mtype)
        : decree(d), from(f), to(t), type(mtype)
    {
    }
};


Message Response(Message message, MessageType type);


#endif
