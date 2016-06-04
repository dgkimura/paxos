#ifndef __MESSAGES_HPP_INCLUDED__
#define __MESSAGES_HPP_INCLUDED__

#include <decree.hpp>
#include <replicaset.hpp>


enum class MessageType
{
    RequestMessage,
    PrepareMessage,
    PromiseMessage,
    AcceptMessage,
    AcceptedMessage,
    UpdateMessage,
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
