# paxos

[![Build Status](https://travis-ci.org/dgkimura/paxos.svg?branch=master)](https://travis-ci.org/dgkimura/paxos)
[![Coverage Status](https://coveralls.io/repos/github/dgkimura/paxos/badge.svg?branch=master)](https://coveralls.io/github/dgkimura/paxos?branch=master)

The paxos project is an implementation of the paxos consensus protocol. It
provides a library for maintaining consistency within a distributed system.


## Build
You will need a compiler with full C++11 support. You can then use CMake to
generate MakeFiles for this project.
```
$ mkdir build && cd build
$ cmake ..
$ make && ./unittests/all_unittests
```


## Setup
Create a `replicaset` file in the directory where the parliament will run.

```
$ echo 127.0.0.1:8080 >> replicaset
```


## Usage
Create a parliament and then send proposals to vote upon.

```cpp
#include <paxos/parliament.hpp>

int main(void)
{
    //
    // Create a parliament group.
    //
    Parliament p(Replica("127.0.0.1", 8080));

    //
    // Sends a proposal to vote on.
    //
    p.SendProposal("Pinky says, 'Narf!'");
}
```

Each passed proposal is appended into the `paxos.ledger`.
```
$ cat paxos.ledger
22 serialization::archive 11 0 0 0 0 0  0 1 0 19 "Narf!", says Pinky
```


## References
- [The Part-Time Parliament](http://research.microsoft.com/en-us/um/people/lamport/pubs/lamport-paxos.pdf)
