#ifndef __MESSAGES_HPP_INCLUDED__
#define __MESSAGES_HPP_INCLUDED__

#include <decree.hpp>


struct Message
{
    Decree decree;
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
