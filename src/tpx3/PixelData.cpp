#include "tpx3.h"

#include <cassert>

using namespace spec_hom;

unsigned long PixelData::memsize() const {

    auto sz = sizeof(*this);
    sz += addr.size() * sizeof(addr[0]);
    sz += toa.size() * sizeof(toa[0]);
    sz += tot.size() * sizeof(tot[0]);

    return sz;

}

unsigned long PixelData::numPackets() const {

    assert((addr.size() == toa.size()) && (addr.size() == tot.size()));
    return addr.size();

}

bool PixelData::isEmpty() const {

    return numPackets() == 0;

}