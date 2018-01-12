#include "paxos/callback.hpp"


namespace paxos
{


Callback::Callback(MessageHandler message_handler_)
{
    message_handler = message_handler_;
}


void
Callback::operator()(Message message)
{
    message_handler(message);
}


}
