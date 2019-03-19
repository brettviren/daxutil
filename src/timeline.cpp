
#include "dax/timeline.hpp"
#include <cstdlib>
#include <assert.h>
#include <vector>

#include <iostream>
using std::cerr;
using std::endl;

dax::Transition::Transition(tick_t tick)
    : m_tick(tick)
{
}



// rhs is to the right of us
void dax::Transition::insert_before(const Transition& rhs)
{
    m_ints.insert(rhs.ints().begin(), rhs.ints().end());
    m_ints.insert(rhs.ends().begin(), rhs.ends().end());
}

// lhs is to the left of us
void dax::Transition::insert_after(const Transition& lhs)
{
    m_ints.insert(lhs.begs().begin(), lhs.begs().end());
    m_ints.insert(lhs.ints().begin(), lhs.ints().end());
}

void dax::Transition::insert(const Transition& lhs, const Transition& rhs)
{
    insert_before(lhs);
    insert_after(rhs);
}

void dax::Transition::add_beg(int n)
{
    m_begs.insert(n);
}
void dax::Transition::add_end(int n)
{
    m_ends.insert(n);
}
void dax::Transition::add_int(int n)
{
    m_ints.insert(n);
}

int first(const dax::Transition::markers_t& marks) {
    if (marks.empty()) { return -1; }
    return *marks.begin();
}

int dax::Transition::as_beg() const
{
    int n = -1;
    if (!m_ints.empty()) { n = std::max(n, first(m_ints)); }
    if (!m_begs.empty()) { n = std::max(n, first(m_begs)); }
    return n;
}

void dax::Transition::dump() const
{
    cerr << "transition t="<<m_tick;
    cerr << "\n\tbegs:";
    for (const auto& it : m_begs) {
        cerr << " " << it;
    }
    cerr << "\n\tints:";
    for (const auto& it : m_ints) {
        cerr << " " << it;
    }
    cerr << "\n\tends:";
    for (const auto& it : m_ends) {
        cerr << " " << it;
    }
    cerr << "\n";
}

int dax::Transition::as_end() const
{
    int n = -1;
    if (!m_ints.empty()) { n = std::max(n, first(m_ints)); }
    if (!m_ends.empty()) { n = std::max(n, first(m_ends)); }
    return n;
}

bool dax::Transition::empty() const
{
    return m_begs.empty() and m_ints.empty() and m_ends.empty();
}

int dax::Transition::remove(int n)
{
    int ret = 0;
    if (m_begs.erase(n)) {
        ret |= (1<<1);
    }
    if (m_ints.erase(n)) {
        ret |= (1<<2);
    }
    if (m_ends.erase(n)) {
        ret |= (1<<3);
    }
    return ret;
}

dax::Timeline::Timeline()
    : m_last(m_timeline.begin())
{
}

dax::Timeline::iterator dax::Timeline::find_after(tick_t tick) 
{
    if (m_timeline.empty()) {
        return end();
    }
    
    if (m_last->second.tick() == tick) {
        return m_last;          // lucky
    }
    iterator maybe = m_timeline.find(tick);
    if (maybe != m_timeline.end()) {
        m_last = maybe;
        return m_last;
    }

    tick_t dt = m_last->second.tick() - tick;
    if (dt > 0) {               // search down
        do {
            --m_last;
            const tick_t dt2 = m_last->second.tick() - tick;
            if (dt2 > 0) {
                dt = dt2;
                continue;
            }
            ++m_last;
            return m_last;
        } while (m_last != begin());
        return end();           // shouldn't reach
    }

    // dt < 0
    ++m_last;
    while (m_last != end()) {   // search up
        const tick_t dt2 = m_last->second.tick() - tick;
        if (dt2 < 0) {
            ++m_last;
            dt = dt2;
            continue;
        }
        // changed sign
        return m_last;
    }

    // shouldn't reach
    return end();

}


void dax::Timeline::add(int n, tick_t tbeg, tick_t tend)
{
    auto got_beg = m_timeline.emplace(tbeg, Transition(tbeg));
    auto got_end = m_timeline.emplace(tend, Transition(tend));
    
    iterator it_beg = got_beg.first;
    iterator it_end = got_end.first;

    it_beg->second.add_beg(n);
    it_end->second.add_end(n);

    // add existing to new
    if (got_beg.second) {
        if (it_beg != begin()) {
            iterator it = it_beg;
            --it;
            it_beg->second.insert_after(it->second);
        }
        if (it_beg != end()) {
            iterator it = it_beg;
            ++it;
            if (it == it_end) {
                ++it;
            }
            if (it != end()) {
                it_beg->second.insert_before(it->second);
            }
        }
    }

    if (got_end.second) {
        if (it_end != begin()) {
            iterator it = it_end;
            --it;
            if (it == it_beg) {
                --it;
            }
            if (it != begin()) {
                it_end->second.insert_after(it->second);
            }
        }
        if (it_end != end()) {
            iterator it = it_end;
            ++it;
            if (it != end()) {
                it_end->second.insert_before(it->second);
            }
        }
    }

    // add new to existing in between
    iterator it = it_beg;
    ++it;
    while (it != it_end and it != end()) {
        it->second.add_int(n);
        ++it;
    }

}

void dax::Timeline::del(int n)
{
    // this is a bit expensive.
    std::vector<tick_t> dead;
    for (auto& it : m_timeline) {
        if (! it.second.remove(n)) {
            continue;
        }
        if (it.second.empty()) {
            dead.push_back(it.first);
        }
    }
    for (auto tick : dead) {
        m_timeline.erase(tick);
    }
}

int dax::Timeline::epoch(tick_t tick) 
{
    iterator it = find_after(tick);
    if (it == end()) {
        return -1;
    }
    return it->second.as_end();
}

void timeline_test (bool verbose)
{
    dax::Timeline tl;
    tl.add(1,100,200);
    tl.add(2,120,220);
    tl.add(3,120,180);
    tl.add(4,190,210);

    cerr << "-------------\n";
    for (const auto& tr : tl) {
        tr.second.dump();
    }
    cerr << "-------------\n";

    assert(1 == tl.epoch(105));
    assert(3 == tl.epoch(125));
    assert(2 == tl.epoch(185));
    assert(4 == tl.epoch(195));
    assert(2 == tl.epoch(215));

    tl.del(3);
        
    assert(1 == tl.epoch(105));
    assert(2 == tl.epoch(125));
    assert(2 == tl.epoch(185));
    assert(4 == tl.epoch(195));
    assert(2 == tl.epoch(215));

    tl.del(1);
        
    assert(-1 == tl.epoch(105));
    assert(2 == tl.epoch(125));
    assert(2 == tl.epoch(185));
    assert(4 == tl.epoch(195));
    assert(2 == tl.epoch(215));

}

