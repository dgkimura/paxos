#include <callback.hpp>


Callback::Callback(MessageHandler message_handler)
{
    message_handler = message_handler;
}


void
Callback::operator()(Message message)
{
    message_handler(message);
}
