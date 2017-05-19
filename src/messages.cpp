#include <paxos/messages.hpp>


Message
Response(Message message, MessageType type)
{
    Message response(message.decree, message.to, message.from, type);
    response.root_decree = message.root_decree;
    return response;
}
