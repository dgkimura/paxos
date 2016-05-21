#include <vector>

#include <decree.hpp>


class Ledger
{
public:

    virtual void Append(Decree decree) = 0;

    virtual bool Contains(Decree decree) = 0;

    virtual void ApplyCheckpoint() = 0;
};


class VolatileLedger : public Ledger
{
public:

    VolatileLedger();

    virtual ~VolatileLedger();

    virtual void Append(Decree decree);

    virtual bool Contains(Decree decree);

    virtual void ApplyCheckpoint();

private:

    std::vector<Decree> decree_entries;
};
