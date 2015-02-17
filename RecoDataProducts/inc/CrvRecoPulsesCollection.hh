#ifndef RecoDataProducts_CrvRecoPulsesCollection_hh
#define RecoDataProducts_CrvRecoPulsesCollection_hh

//
// $Id: $
// $Author: ehrlich $
// $Date: 2014/08/07 01:33:41 $
//
// Contact person Ralf Ehrlich
//

#include "DataProducts/inc/CRSScintillatorBarIndex.hh"
#include "RecoDataProducts/inc/CrvRecoPulses.hh"

#include <map>
#include <set>

namespace mu2e 
{
  typedef std::map<mu2e::CRSScintillatorBarIndex,mu2e::CrvRecoPulses> CrvRecoPulsesCollection;
}

#endif /* RecoDataProducts_CrvRecoPulsesCollection_hh */