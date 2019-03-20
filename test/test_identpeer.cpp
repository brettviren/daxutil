#include "dax/IdentPeer.hpp"

#include <czmq.h>


// run with valgrind.
// valgrind --leak-check=full build/test/test_identpeer 

void dump_ss(std::string name, const dax::IdentPeer::ssmap_t& ss)
{
    zsys_info("peer of: %s", name.c_str());
    for (const auto& it : ss) {
        zsys_info("\t%s %s", it.first.c_str(), it.second.c_str());
    }
}

int main()
{
    zsys_init();
    bool verbose = true;
    {
        dax::IdentPeer p1("one", verbose);
        p1.set_header("key1","val1");
        p1.set_header("key3","val3");
        p1.debut();

        dax::IdentPeer p2("two", verbose);
        p2.set_header("key2","val2");
        p2.set_header("key3","val3");

        p2.debut();

        for (int n=0; n<10; ++n) {
            dax::IdentPeer::ssmap_t ss = p2.peer_names();
            if (ss.empty()) {
                zsys_info("empty: %d", n);
                zclock_sleep(100);
                continue;
            }
            dump_ss("one", ss);
            auto h1 = p2.peer_headers(ss.begin()->first);
            dump_ss("one headers", h1);
            break;
        }
        
    }
    zsys_info("Exiting");


    return 0;
}


