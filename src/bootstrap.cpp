#include "paxos/bootstrap.hpp"


namespace paxos
{


void SendBootstrap(
    std::string local_directory,
    std::string remote_directory,
    std::vector<boost::filesystem::directory_entry> filepaths,
    std::function<void(BootstrapFile)> send_file)
{
    {
        //
        // We must send an empty replicaset file first. This ensures that the
        // replica will ignore any paxos messages send during bootstrap. Else
        // the replica may incorrectly send PROMISE or ACCEPTED messages based
        // on the previous replica set.
        //
        // This also conversely protects the current replica set against the
        // new replica from issuing PREPARE or ACCEPTED messages before the new
        // replica has been fully integrated into the replicset.
        //
        BootstrapFile file;
        boost::filesystem::path remotepath(remote_directory);
        remotepath /= ReplicasetFilename;
        file.name = remotepath.native();
        file.content = "";
        send_file(file);
    }

    for (auto& entry : filepaths)
    {
        if (boost::algorithm::ends_with(entry.path().native(), ReplicasetFilename))
        {
            //
            // Skip any replicaset updates until we are done sending all
            // replicated files.
            //
            continue;
        }

        // 1. serialize file
        std::ifstream filestream(entry.path().native());
        std::stringstream buffer;
        buffer << filestream.rdbuf();

        // 2. send bootstrap file
        BootstrapFile file;
        boost::filesystem::path remotepath(remote_directory);
        remotepath /= entry.path().filename();
        file.name = remotepath.native();
        file.content = buffer.str();
        send_file(file);
    }

    {
        BootstrapFile file;
        boost::filesystem::path remotepath(remote_directory);

        //
        // Let the new replica decide what is the highest proposed decree. This
        // prevents trying to flush a decree from another replica.
        //
        remotepath /= HIGHEST_PROPOSED_DECREE_FILENAME;
        auto proposed = PersistentDecree(local_directory,
                                         HIGHEST_PROPOSED_DECREE_FILENAME).Get();
        proposed.content = "";
        file.name = remotepath.native();
        file.content = Serialize(proposed);
        send_file(file);
    }
    {
        BootstrapFile file;
        boost::filesystem::path remotepath(remote_directory);

        //
        // We must empty the content of accepted decree because actions must be
        // completed before writes to ledger. If we do not, then the accepted
        // decree on the new replica will incorrectly consider current decree as
        // stale accept from a prevous round.
        //
        remotepath /= ACCEPTED_DECREE_FILENAME;
        auto accepted = PersistentDecree(local_directory,
                                         ACCEPTED_DECREE_FILENAME).Get();
        accepted.content = "";
        file.name = remotepath.native();
        file.content = Serialize(accepted);
        send_file(file);
    }
    {
        BootstrapFile file;
        boost::filesystem::path remotepath(remote_directory);

        //
        // Now that the replicated files are all sent we can finally send the
        // "actual" replicaset file allowing the replica to promise and accept
        // messages.
        //
        std::ifstream filestream(local_directory + "/" + ReplicasetFilename);
        std::stringstream buffer;
        buffer << filestream.rdbuf();

        remotepath /= ReplicasetFilename;
        file.name = remotepath.native();
        file.content = buffer.str();
        send_file(file);
    }
}


}
