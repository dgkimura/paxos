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
};


struct PrepareMessage : public Message
{
};


struct PromiseMessage : public Message
{
};


struct AcceptMessage : public Message
{
};


struct AcceptedMessage : public Message
{
};


#endif
