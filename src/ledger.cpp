#include <ledger.hpp>


PersistentLedger::PersistentLedger()
{
    file.open("paxos.ledger", std::ios::app | std::ios::binary);
}


PersistentLedger::~PersistentLedger()
{
    file.close();
}


void
PersistentLedger::Append(Decree decree)
{
    file << decree.content.length();
    file << decree.content;
}


bool
PersistentLedger::Contains(Decree decree)
{
    bool contains_decree = false;
    file.seekg(0, file.end);

    int current = 0, length = file.tellg();

    file.seekg(0, std::ios::beg);
    while (current < length)
    {
        int decree_size, decree_number;

        file >> decree_size;
        file >> decree_number;

        if (decree_number == decree.number)
        {
            contains_decree = true;
            break;
        }

        current = current += decree_size;
        file.seekg(current, std::ios::beg);
    }

    return contains_decree;
}


void
PersistentLedger::ApplyCheckpoint()
{
}


VolatileLedger::VolatileLedger()
    : decrees()
{
}


VolatileLedger::~VolatileLedger()
{
}


void
VolatileLedger::Append(Decree decree)
{
    decrees.push_back(decree);
}


bool
VolatileLedger::Contains(Decree decree)
{
    for (Decree d : decrees)
    {
        if (IsDecreeEqual(d, decree))
        {
            return true;
        }
    }
    return false;
}


void
VolatileLedger::ApplyCheckpoint()
{
    decrees.clear();
}
