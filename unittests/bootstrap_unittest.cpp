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

    auto legislators = std::make_shared<paxos::ReplicaSet>();
    paxos::BootstrapListener<MockServer> listener(legislators, "my-address", 111);
    ASSERT_TRUE(is_action_registered);
}


TEST(BootstrapTest, testSendBootstrapFirstFileSentIsEmptyReplicaSetFile)
{
    std::vector<paxos::BootstrapFile> sent_files;
    auto send_file = [&](paxos::BootstrapFile file)
    {
        sent_files.push_back(file);
    };
    paxos::SendBootstrap(
        "local_directorty",
        "remote_directory",
        std::vector<boost::filesystem::directory_entry>{},
        send_file);

    ASSERT_EQ("remote_directory/paxos.replicaset", sent_files[0].name);
    ASSERT_EQ("", sent_files[0].content);
}


TEST(BootstrapTest, testSendBootstrapPentultimateFileSentIsAcceptedDecreeFile)
{
    std::vector<paxos::BootstrapFile> sent_files;
    auto send_file = [&](paxos::BootstrapFile file)
    {
        sent_files.push_back(file);
    };
    paxos::SendBootstrap(
        "local_directorty",
        "remote_directory",
        std::vector<boost::filesystem::directory_entry>{},
        send_file);

    size_t index = sent_files.size() - 2;
    ASSERT_EQ("remote_directory/paxos.accepted_decree", sent_files[index].name);
}


TEST(BootstrapTest, testSendBootstrapLastFileSentIsActualReplicaSetFile)
{
    std::vector<paxos::BootstrapFile> sent_files;
    auto send_file = [&](paxos::BootstrapFile file)
    {
        sent_files.push_back(file);
    };
    paxos::SendBootstrap(
        "local_directorty",
        "remote_directory",
        std::vector<boost::filesystem::directory_entry>{},
        send_file);

    size_t index = sent_files.size() - 1;
    ASSERT_EQ("remote_directory/paxos.replicaset", sent_files[index].name);
}
