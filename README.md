paxos
=====

[![Build Status](https://travis-ci.org/dgkimura/paxos.svg?branch=master)](https://travis-ci.org/dgkimura/paxos)

The paxos project is an implementation of the paxos consensus protocol. It
provides a library for maintaining consistency within a distributed system.


#### Build
You will need a compiler with full C++11 support. You can then use CMake to
generate MakeFiles for this project.
```
$ mkdir build && cd build
$ cmake ..
$ make && ./unittests/all_unittests
```


#### Usage

```cpp
#include <paxos.hpp>

int main(void)
{
    //
    // Create a parliament group.
    //
    Parliament p;

    //
    // Add voting legislators to the parliament group.
    //
    p.AddLegislator("1.1.1.1", 8081);

    //
    // Set our voting legislator in the parliament group.
    //
    p.SetLegislator("127.0.0.1", 8081);

    //
    // Create a proposal to vote on.
    //
    p.CreateProposal("Pinky says, 'Narf!'");
}
```


#### References
- [The Part-Time Parliament](http://research.microsoft.com/en-us/um/people/lamport/pubs/lamport-paxos.pdf)
