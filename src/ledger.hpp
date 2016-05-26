#include <vector>
#include<fstream>

#include <decree.hpp>


class Ledger
{
public:

    virtual void Append(Decree decree) = 0;

    virtual bool Contains(Decree decree) = 0;

    virtual void ApplyCheckpoint() = 0;
};


class PersistentLedger : public Ledger
{
public:

    PersistentLedger();

    virtual ~PersistentLedger();

    virtual void Append(Decree decree);

    virtual bool Contains(Decree decree);

    virtual void ApplyCheckpoint();

private:
    std::fstream file;
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

    std::vector<Decree> decrees;
};
