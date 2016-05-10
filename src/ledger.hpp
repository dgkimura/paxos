#include <string>

#include <decree.hpp>


class Ledger
{
public:

    Ledger();
    void Append(Decree decree);
    void ApplyCheckpoint();
    ~Ledger();
};
