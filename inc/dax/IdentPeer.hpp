/** dax::IdentPeer -- participate in daxnet peering
 *
 * An application can use an instance of this class to participate in
 * daxnet Zyre-based peering.  It provides stateful caching of Zyre
 * events to provide a synchronous API.
 */  
#ifndef DAX_IDENTPEER
#define DAX_IDENTPEER

#include <czmq.h>
#include <string>

#include <unordered_map>

namespace dax {

    class IdentPeer
    {
    public:
        // create zyre, but do not yet start.
        IdentPeer(std::string nickname, bool verbose=false);
        // destroy zyre peering.
        ~IdentPeer();

        // Set a Zyre header.  
        void set_header(std::string key, std::string value);

        // Launch zyre.  This must be called after all desired headers
        // are set.
        void debut();

        typedef std::unordered_map<std::string, std::string> ssmap_t;

        // return map from uuid to nickname.  Nicknames are not
        // guaranteed unique so are the values.
        ssmap_t peer_names();
        
        // Return all headers for UUID if it's known.
        ssmap_t peer_headers(std::string uuid);
        
        // todo
        // chat


    private:

        zactor_t* m_actor;
    };

}

#endif /* DAX_IDENTPEER */
