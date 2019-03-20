#include "dax/IdentPeer.hpp"

#include <zyre.h>
#include <czmq.h>


typedef std::unordered_map<std::string, std::string> headers_t;

struct peer_t {
    std::string uuid;
    std::string nick;
    std::string addr;
    headers_t headers;
};

// map from uuid to nick
typedef std::unordered_map<std::string, peer_t> peer_map_t;

void fill_enter(peer_map_t& peers, zmsg_t* msg)
{
    char *peer = zmsg_popstr (msg);

    auto& p = peers[peer];
    p.uuid = peer;
    free(peer);
    
    char *name = zmsg_popstr (msg);
    p.nick = name;
    free(name);

    zframe_t* hframe = zmsg_pop(msg);
    zhash_t* hhash = zhash_unpack(hframe);
    char* item = (char*)zhash_first(hhash);
    while (item) {
        p.headers[zhash_cursor(hhash)] = item;
        item = (char*)zhash_next(hhash);
    }
    zframe_destroy(&hframe);

    char* addr = zmsg_popstr (msg);
    p.addr = addr;
    free(addr);
}

void fill_exit(peer_map_t& peers, zmsg_t* msg)
{
    char *peer = zmsg_popstr (msg);
    auto it = peers.find(peer);
    if (it == peers.end()) {
        zsys_warning("IdentPeer exit from unknown: %s", peer);
    }
    else {
        peers.erase(it);
    }
    free(peer);
}

struct args_t {
    std::string nickname;
    bool verbose;
};

static void
ident_peer_actor(zsock_t* pipe, void* vargs)
{
    args_t* args = (args_t*)vargs;

    zyre_t* zyre = zyre_new(args->nickname.c_str());
    if (args->verbose) {
        zyre_set_verbose(zyre);
    }
    zsock_t* zpipe = zyre_socket(zyre);
    peer_map_t peers;
    zpoller_t* poller = zpoller_new(pipe, zpipe, NULL);

    zsock_signal(pipe, 0);      // ready

    bool terminated = false;
    while (!terminated) {
        void* which = zpoller_wait(poller, -1);

        if (!which) {
            zsys_info("IdentPeer %s interrupted", args->nickname.c_str());
            break;
        }

        zmsg_t *msg = zmsg_recv (which);

        if (which == zpipe) {
            char *event = zmsg_popstr (msg);
            
            if (args->verbose) {
                zsys_info("%s: ZYRE event=%s",
                          args->nickname.c_str(), event);
            }
            if (streq (event, "ENTER")) {
                fill_enter(peers, msg);
            }
            if (streq (event, "EXIT")) {
                fill_exit(peers, msg);                
            }
            free (event);
        }

        else if (which == pipe) {

            char* cmd = zmsg_popstr(msg);
            if (args->verbose) {
                zsys_info("%s: API command=%s",
                          args->nickname.c_str(), cmd);
            }
            if (streq(cmd, "SET HEADER")) {
                char* key = zmsg_popstr(msg);
                char* val = zmsg_popstr(msg);
                zyre_set_header(zyre, key, val, NULL);
                free (val);
                free (key);
            }
            else if (streq(cmd, "DEBUT")) {
                zyre_start(zyre);
            }
            else if (streq(cmd, "QUIT")) {
                zyre_stop(zyre);
                terminated = true;
            }
            else if (streq(cmd, "PEER NAMES")) {
                // not really headers, but use same type
                headers_t *pret = new headers_t;
                for (const auto& it : peers) {
                    (*pret)[it.first] = it.second.nick;
                    zsys_info("%s %s", it.first.c_str(), it.second.nick.c_str());
                }
                zsock_send(pipe, "sp", "PEER NAMES", pret);
                pret = 0;
            }
            else if (streq(cmd, "PEER HEADERS")) {
                char* uuid = zmsg_popstr(msg);
                headers_t *pret = new headers_t;
                for (const auto& it : peers[uuid].headers) {
                    (*pret)[it.first] = it.second;
                    zsys_info("%s %s", it.first.c_str(), it.second.c_str());
                }
                zsock_send(pipe, "ssp", "PEER HEADERS", uuid, pret);
                pret = 0;
                free(uuid);
            }
            else {
                zsys_warning("IdentPeer: unkown command %s", cmd);
            }
            free (cmd);
        }

        zmsg_destroy (&msg);

    }
    zpoller_destroy(&poller);
    zyre_destroy(&zyre);
    delete(args);
}

dax::IdentPeer::IdentPeer(std::string nickname, bool verbose)
    : m_actor(zactor_new(ident_peer_actor, (void*)new args_t{nickname, verbose}))
{

}

dax::IdentPeer::~IdentPeer()
{
    zsock_send(zactor_sock(m_actor), "s", "QUIT");

    zactor_destroy(&m_actor);
}

void dax::IdentPeer::set_header(std::string key, std::string value)
{
    zsock_send(zactor_sock(m_actor), "sss",
               "SET HEADER", key.c_str(), value.c_str());

}
void dax::IdentPeer::debut()
{
    zsock_send(zactor_sock(m_actor), "s", "DEBUT");
}


dax::IdentPeer::ssmap_t dax::IdentPeer::peer_names()
{
    const char* method = "PEER NAMES";
    zsock_send(zactor_sock(m_actor), "s", method);
    dax::IdentPeer::ssmap_t* p=0, copy;
    char* back=0;
    zsock_recv(zactor_sock(m_actor), "sp", &back, &p);
    assert(streq(back, method));
    free(back);
    copy = *p;
    delete p;
    p = 0;
    return copy;

}

dax::IdentPeer::ssmap_t dax::IdentPeer::peer_headers(std::string uuid)
{
    const char* method = "PEER HEADERS";
    zsock_send(zactor_sock(m_actor), "ss", method, uuid.c_str());
    dax::IdentPeer::ssmap_t* p=0, copy;
    char* back=0;
    char* back2=0;
    zsock_recv(zactor_sock(m_actor), "ssp", &back, &back2, &p);
    assert(streq(back, method));
    assert(streq(back2, uuid.c_str()));
    free(back);
    free(back2);
    copy = *p;
    delete p;
    p = 0;
    return copy;
}

