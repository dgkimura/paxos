#ifndef __LEDGER_HPP_INCLUDED__
#define __LEDGER_HPP_INCLUDED__

#include <decree.hpp>
#include <logging.hpp>
#include <queue.hpp>


class Ledger
{
public:

    Ledger(std::shared_ptr<BaseQueue<Decree>> decrees_);

    ~Ledger();

    void Append(Decree decree);

    void Remove();

    int Size();

    Decree Head();

    Decree Tail();

    Decree Next(Decree previous);

private:

    std::shared_ptr<BaseQueue<Decree>> decrees;
};


#endif
