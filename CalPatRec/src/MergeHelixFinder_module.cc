///////////////////////////////////////////////////////////////////////////////
// $Id: $
// $Author: $ 
// $Date: $
// takes inputs from two helix finding algorithms, produces one helix collection 
// on output to be used for the track seed-fit
//
// Original author P. Murat
//
//
///////////////////////////////////////////////////////////////////////////////
// framework
#include "art/Framework/Principal/Event.h"
#include "fhiclcpp/ParameterSet.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Core/EDProducer.h"
#include "art/Framework/Core/ModuleMacros.h"
// BaBar
#include "BTrk/BaBar/BaBar.hh"
#include "BTrkData/inc/TrkStrawHit.hh"
#include "BTrk/ProbTools/ChisqConsistency.hh"
#include "BTrk/BbrGeom/BbrVectorErr.hh"
#include "BTrk/KalmanTrack/KalHit.hh"
#include "BTrk/BbrGeom/TrkLineTraj.hh"
#include "BTrk/TrkBase/TrkPoca.hh"
#include "BTrk/TrkBase/HelixParams.hh"

#include "GeometryService/inc/GeometryService.hh"
#include "GeometryService/inc/GeomHandle.hh"
#include "GeometryService/inc/VirtualDetector.hh"
#include "GeometryService/inc/DetectorSystem.hh"

#include "CalorimeterGeom/inc/Calorimeter.hh"

#include "TROOT.h"
#include "TFolder.h"
#include "TVector2.h"

#include "RecoDataProducts/inc/HelixSeed.hh"

#include "ConfigTools/inc/ConfigFileLookupPolicy.hh"

#include "TrkDiag/inc/KalDiag.hh"
#include "BTrkData/inc/Doublet.hh"
#include "TrkReco/inc/DoubletAmbigResolver.hh"

// CalPatRec
// #include "CalPatRec/inc/TrkDefHack.hh"
#include "Mu2eUtilities/inc/LsqSums4.hh"
#include "CalPatRec/inc/ObjectDumpUtils.hh"

#include "RecoDataProducts/inc/AlgorithmIDCollection.hh"
// Xerces XML Parser
#include <xercesc/dom/DOM.hpp>

//CLHEP
#include "CLHEP/Units/PhysicalConstants.h"
#include "CLHEP/Vector/ThreeVector.h"
// root 
#include "TMath.h"
#include "TFile.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TH3F.h"
// C++
#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <functional>
#include <float.h>
#include <vector>
#include <set>
#include <map>
using namespace std; 
using CLHEP::Hep3Vector;

namespace mu2e {
  class MergeHelixFinder : public art::EDProducer {
  public:
    explicit MergeHelixFinder(fhicl::ParameterSet const&);
    virtual ~MergeHelixFinder();
    virtual void beginJob();
    virtual void beginRun(art::Run&);
    virtual void produce(art::Event& event ); 
    void endJob();
    
  private:
    unsigned         _iev;
					// configuration parameters
    int              _diag;
    int              _debugLevel;
    float            _minTprChi2;
    float            _minCprChi2;
    int              _printfreq;
    bool             _addhits; 
					// event object labels
    std::string      _trkHelixFinderModuleLabel;
    std::string      _calHelixFinderModuleLabel;
  };
  
  MergeHelixFinder::MergeHelixFinder(fhicl::ParameterSet const& pset) :
    _diag                        (pset.get<int>("diagLevel" )),
    _debugLevel                  (pset.get<int>("debugLevel")),
    _trkHelixFinderModuleLabel   (pset.get<std::string>("trkHelixFinderModuleLabel"   )),
    _calHelixFinderModuleLabel   (pset.get<std::string>("calHelixFinderModuleLabel"   ))
  {

    produces<AlgorithmIDCollection>  ();
    produces<HelixSeedCollection>    ();
    
  }

  MergeHelixFinder::~MergeHelixFinder() {
  }
  
  void MergeHelixFinder::beginJob() {
  }
  
  void MergeHelixFinder::beginRun(art::Run& ) {
  }
  


//-----------------------------------------------------------------------------
  void MergeHelixFinder::produce(art::Event& AnEvent) {

					// assume less than 100 tracks
    int const   max_ntrk(100);
    int         tpr_flag[max_ntrk], cpr_flag[max_ntrk], ntpr(0), ncpr(0);

    art::Handle<mu2e::HelixSeedCollection>    tpr_h, cpr_h;

    mu2e::HelixSeedCollection  *list_of_helices_tpr(0), *list_of_helices_cpr(0);

    mu2e::GeomHandle<mu2e::DetectorSystem>      ds;
    mu2e::GeomHandle<mu2e::VirtualDetector>     vdet;

    unique_ptr<AlgorithmIDCollection>  algs     (new AlgorithmIDCollection );
    unique_ptr<HelixSeedCollection>    helixPtrs(new HelixSeedCollection   );

    if (_debugLevel > 0) ObjectDumpUtils::printEventHeader(&AnEvent,"MergeHelixFinder::produce");

    AnEvent.getByLabel(_trkHelixFinderModuleLabel,tpr_h);
    AnEvent.getByLabel(_calHelixFinderModuleLabel,cpr_h);
    
    if (tpr_h.isValid()) { 
      list_of_helices_tpr = (mu2e::HelixSeedCollection*) &(*tpr_h);
      ntpr                = list_of_helices_tpr->size();
    }

    if (cpr_h.isValid()) {
      list_of_helices_cpr = (mu2e::HelixSeedCollection*) &(*cpr_h);
      ncpr                = list_of_helices_cpr->size();
    }

    for (int i=0; i<max_ntrk; i++) {
      tpr_flag[i] = 1;
      cpr_flag[i] = 1;
    }

    const HelixSeed          *helix_tpr, *helix_cpr;
    short                     best(-1),  mask;
    AlgorithmID               alg_id;
    HelixHitCollection        tlist, clist;
    int                       nat, nac, natc;
    const mu2e::HelixHit     *hitt, *hitc;

    for (int i1=0; i1<ntpr; i1++) {
      helix_tpr    = &list_of_helices_tpr->at(i1);
      mask         = 1 << AlgorithmID::TrkPatRecBit;
      tlist        = helix_tpr->hits();
      nat          = tlist.size();
      natc         = 0;

      for (int i2=0; i2<ncpr; i2++) {
	helix_cpr    = &list_of_helices_cpr->at(i2);
	clist        = helix_cpr->hits();
	nac          = clist.size();

//-----------------------------------------------------------------------------
// check the number of common hits: do we need to check also if they have 
// close momentum?
//-----------------------------------------------------------------------------
	for(size_t k=0; k<tlist.size(); ++k){ 
	  hitt = &tlist.at(k);
	  for(size_t l=0; l<clist.size(); l++){ 
	    hitc = &clist.at(l);
	    if (hitt->index() == hitc->index()) {
	      natc += 1;
	      break;
	    }
	  }
	}
//-----------------------------------------------------------------------------
// if > 50% of all hits are common, consider cpr and tpr to be the same
// logic of the choice: 
// 1. take the track which has more hits
// 2. if two tracks have the same number of active hits, choose the one with 
//    best chi2
//-----------------------------------------------------------------------------
	if (natc > (nac+nat)/4.) {

	  mask = mask | (1 << AlgorithmID::CalPatRecBit);

//-----------------------------------------------------------------------------
// tracks sahre more than 50% of their hits, in this case take the one from CPR
//-----------------------------------------------------------------------------
	  helixPtrs->push_back(*helix_cpr);
	  best    = AlgorithmID::CalPatRecBit;

	  tpr_flag[i1] = 0;
	  cpr_flag[i2] = 0;
	  break;
	}
      }

      if (tpr_flag[i1] == 1) {
	helixPtrs->push_back(*helix_tpr);
	best = AlgorithmID::TrkPatRecBit;
      }

      alg_id.Set(best,mask);
      algs->push_back(alg_id);
    }
//-----------------------------------------------------------------------------
// account for presence of multiple tracks
//-----------------------------------------------------------------------------
    for (int i=0; i<ncpr; i++) {
      if (cpr_flag[i] == 1) {
	helix_cpr = &list_of_helices_cpr->at(i);

	helixPtrs->push_back(*helix_cpr);

	best = AlgorithmID::CalPatRecBit;
	mask = 1 << AlgorithmID::CalPatRecBit;

	alg_id.Set(best,mask);
	algs->push_back(alg_id);
      }
    }


    AnEvent.put(std::move(helixPtrs));
    AnEvent.put(std::move(algs     ));
  }


//-----------------------------------------------------------------------------
// end job : 
//-----------------------------------------------------------------------------
  void MergeHelixFinder::endJob() {
  }
  
}

using mu2e::MergeHelixFinder;
DEFINE_ART_MODULE(MergeHelixFinder);
