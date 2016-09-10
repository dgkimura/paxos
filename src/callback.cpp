#include "paxos/callback.hpp"


Callback::Callback(MessageHandler message_handler_)
{
    message_handler = message_handler_;
}


void
Callback::operator()(Message message)
{
    message_handler(message);
}
