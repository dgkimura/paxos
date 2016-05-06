#ifndef __CALLBACK_HPP_INCLUDED__
#define __CALLBACK_HPP_INCLUDED__


#include <functional>

#include <context.hpp>
#include <messages.hpp>


using MessageHandler = std::function<void(Message message)>;

class Callback
{
public:

    Callback(MessageHandler message_handler);

    void operator()(Message message);

private:

    MessageHandler message_handler;
};


#endif
