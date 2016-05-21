#include <ledger.hpp>


VolatileLedger::VolatileLedger()
    : decree_entries()
{
}


VolatileLedger::~VolatileLedger()
{
}


void
VolatileLedger::Append(Decree decree)
{
    decree_entries.push_back(decree);
}


bool
VolatileLedger::Contains(Decree decree)
{
    for (Decree d : decree_entries)
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
    decree_entries.clear();
}
