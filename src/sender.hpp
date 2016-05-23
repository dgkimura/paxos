#include <vector>

#include <messages.hpp>
#include <replicaset.hpp>

class Sender
{
public:

    virtual void Reply(Message message) = 0;

    virtual void ReplyAll(Message message) = 0;
};


class FakeSender : public Sender
{
public:

    FakeSender()
        : sent_messages(),
          full_replicaset()
    {
    }

    FakeSender(ReplicaSet full_replicaset_)
        : sent_messages(),
          full_replicaset(full_replicaset_)
    {
    }

    ~FakeSender()
    {
    }

    void Reply(Message message)
    {
        sent_messages.push_back(message);
    }

    void ReplyAll(Message message)
    {
        for (auto r : full_replicaset)
        {
            sent_messages.push_back(message);
        }
    }

    std::vector<Message> sentMessages()
    {
        return sent_messages;
    }

private:

    std::vector<Message> sent_messages;

    ReplicaSet full_replicaset;
};
