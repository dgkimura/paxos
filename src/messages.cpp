#include <paxos/messages.hpp>


namespace paxos
{


Message
Response(Message message, MessageType type)
{
    Message response(message.decree, message.to, message.from, type);
    return response;
}


}
