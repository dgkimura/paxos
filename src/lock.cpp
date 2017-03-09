#include <boost/filesystem.hpp>

#include "paxos/decree.hpp"
#include "paxos/lock.hpp"
#include "paxos/logging.hpp"
#include "paxos/messages.hpp"
#include "paxos/serialization.hpp"


distributed_lock::distributed_lock(
    Replica replica,
    std::shared_ptr<Sender> sender,
    std::string location,
    std::string lockname)
    : replica(replica),
      sender(sender),
      location(location),
      lockname(lockname),
      signal(std::make_shared<Signal>())
{
}


void
distributed_lock::lock()
{
    while (!is_locked())
    {
        Decree d;
        d.type = DecreeType::DistributedLockDecree;
        d.content = Serialize(
            DistributedLockDecree
            {
                true
            }
        );
        sender->Reply(
            Message(
                d,
                replica,
                replica,
                MessageType::RequestMessage)
        );
        signal->Wait();
    }
}


void
distributed_lock::unlock()
{
    if (is_locked())
    {
        Decree d;
        d.type = DecreeType::DistributedLockDecree;
        d.content = Serialize(
            DistributedLockDecree
            {
                false
            }
        );
        sender->Reply(
            Message(
                d,
                replica,
                replica,
                MessageType::RequestMessage)
        );
    }
}


bool
distributed_lock::is_locked()
{
    if (boost::filesystem::exists(
            (boost::filesystem::path(location) /
             boost::filesystem::path(lockname)).string()))
    {
        std::ifstream lockfile(
            (boost::filesystem::path(location) /
             boost::filesystem::path(lockname)).string());

        Replica owner = Deserialize<Replica>(lockfile);
        return owner.hostname == replica.hostname &&
               owner.port == replica.port;
    }
    return false;
}
