#include <functional>

#include "gtest/gtest.h"

#include "paxos/bootstrap.hpp"
#include "paxos/serialization.hpp"


TEST(BootstrapTest, testBootstrapListenerRegistersAction)
{
    static bool is_action_registered = false;

    class MockServer
    {
    public:
        MockServer(std::string address, short port)
        {
        }
        void RegisterAction(std::function<void(std::string content)> action)
        {
            is_action_registered = true;
        }
        void Start()
        {
        }
    };

    auto legislators = std::make_shared<ReplicaSet>();
    BootstrapListener<MockServer> listener(legislators, "my-address", 111);
    ASSERT_TRUE(is_action_registered);
}
