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
Create a `paxos.replicaset` file in the directory where the parliament will run.

```
$ echo 127.0.0.1:8080 >> paxos.replicaset
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

Each passed proposal is appended to the `paxos.ledger`. If you want to process
the decree, you must create your parliament with a decree handler.

```cpp
    Parliament p(
        Replica("127.0.0.1", 8080),
        ".",
        [](std::string decree)
        {
            std::cout << "Processing decree: " << decree << std::endl;
        });
```
As soon as the instance sees the decree passes it will execute the decree your
decree handler and append the decree to the ledger.


## References
- [The Part-Time Parliament](http://research.microsoft.com/en-us/um/people/lamport/pubs/lamport-paxos.pdf)
