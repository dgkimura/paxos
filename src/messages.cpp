#include <messages.hpp>


Message
Response(Message message, MessageType type)
{
    Message response(message.decree, message.to, message.from, type);
    return response;
}
