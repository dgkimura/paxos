#ifndef __MESSAGES_HPP_INCLUDED__
#define __MESSAGES_HPP_INCLUDED__

#include <decree.hpp>
#include <replicaset.hpp>


struct Message
{
    Decree decree;
    Replica from;
    Replica to;

    Message(Decree d, Replica f, Replica t)
        : decree(d), from(f), to(t)
    {
    }
};


struct RequestMessage : public Message
{
    RequestMessage(Decree d, Replica f, Replica t)
        : Message(d, f, t)
    {
    }
};


struct PrepareMessage : public Message
{
    PrepareMessage(Decree d, Replica f, Replica t)
        : Message(d, f, t)
    {
    }
};


struct PromiseMessage : public Message
{
    PromiseMessage(Decree d, Replica f, Replica t)
        : Message(d, f, t)
    {
    }
};


struct AcceptMessage : public Message
{
    AcceptMessage(Decree d, Replica f, Replica t)
        : Message(d, f, t)
    {
    }
};


struct AcceptedMessage : public Message
{
    AcceptedMessage(Decree d, Replica f, Replica t)
        : Message(d, f, t)
    {
    }
};


struct UpdateMessage : public Message
{
    UpdateMessage(Decree d, Replica f, Replica t)
        : Message(d, f, t)
    {
    }
};


struct UpdatedMessage : public Message
{
    UpdatedMessage(Decree d, Replica f, Replica t)
        : Message(d, f, t)
    {
    }
};


template <typename T>
T Response(Message message)
{
    T response(message.decree, message.to, message.from);
    return response;
}


#endif
