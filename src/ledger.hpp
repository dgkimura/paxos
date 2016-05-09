#include <string>


struct Decree
{
    std::string Contents;
    unsigned long int Number;
};


class Ledger
{
public:

    Ledger();
    void Append(Decree decree);
    void ApplyCheckpoint();
    ~Ledger();
};
