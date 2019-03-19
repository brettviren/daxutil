/** dax::Timeline --  class Timeline in namespace dax
 */  
#ifndef DAX_TIMELINE
#define DAX_TIMELINE

#include <set>
#include <map>
#include <cstdint>              // int64_t

namespace dax {

    // An integral tick of a 64 bit clock.  This is like an index and
    // so any ranges expressed as a piar of ticks will be half open.
    // The end tick is one past the end of the range.
    typedef int64_t tick_t;


    // A transition marks a tick with three sets of epoch numbers.
    // The begin set includes all epochs that start on this tick,
    // similar for the end set.  The "ints" number all epochs in which
    // this tick is internal.
    class Transition
    {
    public:
        struct BiggestFirst {
            bool operator() (const int& lhs, const int& rhs) const
                {return lhs>rhs;}
        };
        typedef std::set<int, BiggestFirst> markers_t;
        Transition(tick_t tick);
        
        void dump() const;

        // Fill markers assuming this transition is neither left nor
        // right and is the only one between.
        void insert(const Transition& left, const Transition& right);
        // insert self as if rhs is to the right of us
        void insert_before(const Transition& rhs);
        // insert self as if lhs is to the left of us
        void insert_after(const Transition& lhs);

        // Add epoch number n as a begin or end marker.
        void add_beg(int n);
        void add_end(int n);
        void add_int(int n);

        const markers_t& begs() const { return m_begs; }
        const markers_t& ints() const { return m_ints; }
        const markers_t& ends() const { return m_ends; }
        tick_t tick() const { return m_tick; }
        
        // Return epoch assuming this transition is a beginning.  It's
        // max epoch number of begs and ints.
        int as_beg() const;

        // Return epoch just before this transition.  It's max epoch
        // number of ints and ends.
        int as_end() const;

        // True if no markers are held.
        bool empty() const;

        // remove any markers of n, return bit mask of removed [end,int,beg].
        int remove(int n);

    private:
        tick_t m_tick;
        markers_t m_begs, m_ints, m_ends;
        
    };

    // The timeline maintains an ordered list of transitions.
    class Timeline {
    public:
        typedef std::map<tick_t, Transition> timeline_t;
        typedef timeline_t::iterator iterator;

        Timeline();

        // Add an epoch which spans from beg to end to the timeline.
        void add(int n, tick_t tbeg, tick_t tend);

        // Remove an epoch from the timeline.
        void del(int n);

        // Return the epoch number applicable to the given tick.
        int epoch(tick_t tick);

        iterator find_after(tick_t tick);

        // range interface

        iterator begin() { return m_timeline.begin(); }
        iterator end() { return m_timeline.end(); }

        bool empty() { return m_timeline.empty(); }
        
    private:
        timeline_t m_timeline;
        iterator m_last;
    };


}

#ifdef __cplusplus
extern "C" {
#endif

    void timeline_test (bool verbose);

#ifdef __cplusplus
}
#endif


#endif /* DAX_TIMELINE */
