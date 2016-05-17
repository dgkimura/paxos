#include <messages.hpp>

class Sender
{
public:

    Sender();

    ~Sender();

    template <typename T>
    void Reply(Message message)
    {
    }

    template <typename T>
    void ReplyAll(Message message)
    {
    }
};
