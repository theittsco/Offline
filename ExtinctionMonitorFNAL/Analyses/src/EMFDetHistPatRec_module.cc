// ExtMonFNAL PatRec efficiency/fake rate analysis
//
// Andrei Gaponenko, 2012

#include <iostream>
#include <string>
#include <vector>
#include <set>

#include "fhiclcpp/ParameterSet.h"
#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Core/FindMany.h"
#include "art/Utilities/InputTag.h"

#include "RecoDataProducts/inc/ExtMonFNALTrkParam.hh"
#include "RecoDataProducts/inc/ExtMonFNALTrkParamCollection.hh"
#include "RecoDataProducts/inc/ExtMonFNALPatRecTrackAssns.hh"
#include "RecoDataProducts/inc/ExtMonFNALRecoClusterCollection.hh"

#include "MCDataProducts/inc/SimParticle.hh"
#include "MCDataProducts/inc/SimParticleCollection.hh"
#include "MCDataProducts/inc/ExtMonFNALRecoClusterTruthAssn.hh"
#include "MCDataProducts/inc/ExtMonFNALPatRecTruthAssns.hh"

#include "ExtinctionMonitorFNAL/Geometry/inc/ExtMonFNAL.hh"
#include "GeometryService/inc/GeomHandle.hh"
#include "ExtinctionMonitorFNAL/Reconstruction/inc/PixelRecoUtils.hh"
#include "ExtinctionMonitorFNAL/Analyses/inc/EMFPatRecEffHistograms.hh"

#include "ExtinctionMonitorFNAL/Reconstruction/inc/TrackExtrapolator.hh"

#include "art/Framework/Services/Optional/TFileService.h"
#include "art/Framework/Services/Registry/ServiceHandle.h"

#include "TH1D.h"
#include "TH2D.h"


#define AGDEBUG(stuff) do { std::cerr<<"AG: "<<__FILE__<<", line "<<__LINE__<<", func "<<__func__<<": "<<stuff<<std::endl; } while(0)
//#define AGDEBUG(stuff)

namespace mu2e {
  namespace ExtMonFNAL {

    //================================================================
    class EMFDetHistPatRec : public art::EDAnalyzer {
      std::string patRecModuleLabel_;
      std::string patRecInstanceName_;
      std::string trkTruthModuleLabel_;
      std::string trkTruthInstanceName_;
      std::string clusterTruthModuleLabel_;
      std::string clusterTruthInstanceName_;
      std::string particleModuleLabel_;
      std::string particleInstanceName_;

      std::string geomModuleLabel_; // emtpy to take info from Run
      std::string geomInstanceName_;

      unsigned cutParticleMinClusters_; //

      // signal particle coordinates at first and last plane must be within the limits
      double cutHitXmax_;
      double cutHitYmax_;

      const ExtMonFNAL::ExtMon *extmon_;
      TrackExtrapolator extrapolator_;

      //----------------
      TH2D *hMultiplicitySignal_;
      TH2D *hCommonClusters_;
      EMFPatRecEffHistograms effPhysics_;
      EMFPatRecEffHistograms effSoftware_;

      bool signalParticlePhysics(const SimParticle& particle);
      bool inAcceptance(const ExtMonFNALTrkParam& par);

      bool signalParticleSofware(const art::FindMany<ExtMonFNALRecoCluster,ExtMonFNALRecoClusterTruthBits>& clusterFinder,
                                 unsigned iParticle);

      //----------------------------------------------------------------
      // cuts tuning: single particle mode
      bool singleParticleMode_;
      std::string clusterModuleLabel_; // used only in single particle mode
      std::string clusterInstanceName_; // used only in single particle mode
      bool acceptSingleParticleEvent(const art::Event& event);

    public:
      explicit EMFDetHistPatRec(const fhicl::ParameterSet& pset);
      virtual void beginRun(const art::Run& run);
      virtual void analyze(const art::Event& event);
    };

    //================================================================
    EMFDetHistPatRec::EMFDetHistPatRec(const fhicl::ParameterSet& pset)
      : patRecModuleLabel_(pset.get<std::string>("patRecModuleLabel"))
      , patRecInstanceName_(pset.get<std::string>("patRecInstanceName", ""))
      , trkTruthModuleLabel_(pset.get<std::string>("trkTruthModuleLabel"))
      , trkTruthInstanceName_(pset.get<std::string>("trkTruthInstanceName", ""))
      , clusterTruthModuleLabel_(pset.get<std::string>("clusterTruthModuleLabel"))
      , clusterTruthInstanceName_(pset.get<std::string>("clusterTruthInstanceName", ""))
      , particleModuleLabel_(pset.get<std::string>("particleModuleLabel"))
      , particleInstanceName_(pset.get<std::string>("particleInstanceName", ""))

      , geomModuleLabel_(pset.get<std::string>("geomModuleLabel"))
      , geomInstanceName_(pset.get<std::string>("geomInstanceName", ""))

        // Signal particle cuts
      , cutParticleMinClusters_(pset.get<unsigned>("cutParticleMinClusters"))

      , cutHitXmax_(pset.get<double>("cutHitXmax"))
      , cutHitYmax_(pset.get<double>("cutHitYmax"))

      , extmon_()
      , extrapolator_(extmon_)

      , hMultiplicitySignal_()
      , hCommonClusters_()
      , effPhysics_(pset.get<unsigned>("cutMinCommonClusters"))
      , effSoftware_(pset.get<unsigned>("cutMinCommonClusters"))

      , singleParticleMode_(pset.get<bool>("singleParticleMode", false))
      , clusterModuleLabel_(singleParticleMode_ ? pset.get<std::string>("singleParticleClusterModuleLabel") : "")
      , clusterInstanceName_(pset.get<std::string>("singleParticleClusterInstanceName", ""))
    {
      if(singleParticleMode_) {
        std::cout<<"EMFDetHistPatRec: working in the single particle mode"<<std::endl;
      }
    }

    //================================================================
    void EMFDetHistPatRec::beginRun(const art::Run& run) {
      if(!geomModuleLabel_.empty()) {
        art::Handle<ExtMonFNAL::ExtMon> emf;
        run.getByLabel(geomModuleLabel_, geomInstanceName_, emf);
        extmon_ = &*emf;
      }
      else {
        GeomHandle<ExtMonFNAL::ExtMon> emf;
        extmon_ = &*emf;
      }

      extrapolator_ = TrackExtrapolator(extmon_);

      //----------------------------------------------------------------
      art::ServiceHandle<art::TFileService> tfs;

      hMultiplicitySignal_ = tfs->make<TH2D>("multiplicitySignal", "Num PatRec tracks vs num signal SimParticles",
                                             200, -0.5, 199.5, 200, -0.5, 199.5);

      hMultiplicitySignal_->SetOption("colz");
      hMultiplicitySignal_->GetXaxis()->SetTitle("num signal particles");
      hMultiplicitySignal_->GetYaxis()->SetTitle("num PatRec tracks");


      hCommonClusters_ = tfs->make<TH2D>("commonClusters", "Track&SimParticle vs SimParticle clusters for best track",
                                         10, -0.5, 9.5, 10, -0.5, 9.5);

      hCommonClusters_->SetOption("colz");
      hCommonClusters_->GetXaxis()->SetTitle("particle clusters");
      hCommonClusters_->GetYaxis()->SetTitle("common clusters");

      effPhysics_.book(*extmon_, "effPhysics");
      effSoftware_.book(*extmon_, "effSoftware");
    }

    //================================================================
    void EMFDetHistPatRec::analyze(const art::Event& event) {

      if(singleParticleMode_ && !acceptSingleParticleEvent(event)) {
        return;
      }

      art::Handle<SimParticleCollection> ih;
      event.getByLabel(particleModuleLabel_, particleInstanceName_, ih);

      // FIXME: need to create sequence of SimParticles by hand
      // because map_vector does not work with FindMany
      // https://cdcvs.fnal.gov/redmine/issues/2967
      std::vector<art::Ptr<SimParticle> > particles;
      for(SimParticleCollection::const_iterator i = ih->begin(), iend = ih->end(); i != iend; ++i) {
        particles.push_back(art::Ptr<SimParticle>(ih, i->first.asUint()));
      }

      art::FindMany<ExtMonFNALTrkParam,ExtMonFNALTrkMatchInfo>
        trackFinder(particles, event, art::InputTag(trkTruthModuleLabel_, trkTruthInstanceName_));

      art::FindMany<ExtMonFNALRecoCluster,ExtMonFNALRecoClusterTruthBits>
        clusterFinder(particles, event, art::InputTag(clusterTruthModuleLabel_, clusterTruthInstanceName_));

      // different denominator definitions
      std::set<unsigned> signalPhysics;
      std::set<unsigned> signalSW;

      for(unsigned ip=0; ip<particles.size(); ++ip) {
        if(signalParticlePhysics(*particles[ip])) {
          signalPhysics.insert(ip);

          //----------------
          if(true) { // fill an extra histogam
            const std::vector<const ExtMonFNALTrkMatchInfo*>& matchInfo = trackFinder.data(ip);
            // Figure out the best match
            unsigned maxCommonClusters(0), numClustersOnParticle(0);
            for(unsigned itrack = 0; itrack < matchInfo.size(); ++itrack) {
              if(maxCommonClusters < matchInfo[itrack]->nCommonClusters()) {
                maxCommonClusters =  matchInfo[itrack]->nCommonClusters();
                numClustersOnParticle = matchInfo[itrack]->nParticleClusters();
              }
            }
            hCommonClusters_->Fill(numClustersOnParticle, maxCommonClusters);
          }

          //----------------
          if(signalParticleSofware(clusterFinder, ip)) {
            signalSW.insert(ip);
          }

        } // if(physics)
      } // for(ip)

      art::Handle<ExtMonFNALTrkParamCollection> tracks;
      event.getByLabel(patRecModuleLabel_, patRecInstanceName_, tracks);
      hMultiplicitySignal_->Fill(signalPhysics.size(), tracks->size());

      //----------------------------------------------------------------
      EMFPatRecEffHistograms::Fillable phys =
        effPhysics_.fillable(particles,
                             event,
                             art::InputTag(trkTruthModuleLabel_, trkTruthInstanceName_),
                             signalPhysics.size());

      for(std::set<unsigned>::const_iterator i=signalPhysics.begin(); i != signalPhysics.end(); ++i) {
        phys.fill(*i);
      }

      //----------------------------------------------------------------
      EMFPatRecEffHistograms::Fillable sw =
        effSoftware_.fillable(particles,
                              event,
                              art::InputTag(trkTruthModuleLabel_, trkTruthInstanceName_),
                              // Use the same X axis for both physics and SW plots
                              signalPhysics.size());

      for(std::set<unsigned>::const_iterator i=signalSW.begin(); i != signalSW.end(); ++i) {
        sw.fill(*i);
      }

    } // analyze()

    //================================================================
    bool EMFDetHistPatRec::signalParticlePhysics(const SimParticle& particle) {
      bool res = false;

      const CLHEP::Hep3Vector& startPos = extmon_->mu2eToExtMon_position(particle.startPosition());
      const CLHEP::Hep3Vector& startMom = extmon_->mu2eToExtMon_momentum(particle.startMomentum());

      // Make sure tha particle starts at a place where it can go through the whole detector
      if(extmon_->up().sensor_zoffset().back() < startPos.z()) {

        const double rTrack = extmon_->spectrometerMagnet().trackBendRadius(startMom.mag());
        ExtMonFNALTrkParam mcpar;
        mcpar.setz0(startPos.z());
        mcpar.setposx(startPos.x());
        mcpar.setposy(startPos.y());
        mcpar.setslopex(startMom.x()/startMom.z());
        mcpar.setslopey(startMom.y()/startMom.z());
        mcpar.setrinv(1./rTrack);

        res =  extrapolator_.extrapolateToPlane(extmon_->nplanes()-1, &mcpar) &&
          inAcceptance(mcpar) &&
          extrapolator_.extrapolateToPlane(0, &mcpar) &&
          inAcceptance(mcpar)
          ;
      }

      return res;
    }

    //================================================================
    bool EMFDetHistPatRec::inAcceptance(const ExtMonFNALTrkParam& par) {
      return
        (std::abs(par.posx()) < cutHitXmax_) &&
        (std::abs(par.posy()) < cutHitYmax_);
    }


    //================================================================
    bool EMFDetHistPatRec::signalParticleSofware(const art::FindMany<ExtMonFNALRecoCluster,ExtMonFNALRecoClusterTruthBits>& clusterFinder,
                                                 unsigned iParticle)
    {
      std::set<unsigned> hitPlanes;
      typedef std::vector<const ExtMonFNALRecoCluster*> Clusters;
      const Clusters& clusters = clusterFinder.at(iParticle);
      for(Clusters::const_iterator i=clusters.begin(); i!=clusters.end(); ++i) {
        hitPlanes.insert( (*i)->plane());
      }
      return hitPlanes.size() == extmon_->nplanes();
    }


    //================================================================
    bool EMFDetHistPatRec::acceptSingleParticleEvent(const art::Event& event) {
      art::Handle<ExtMonFNALRecoClusterCollection> coll;
      event.getByLabel(clusterModuleLabel_, clusterInstanceName_, coll);
      return perfectSingleParticleEvent(*coll, extmon_->nplanes());
    }


    //================================================================
  } // namespace ExtMonFNAL
} // namespace mu2e

DEFINE_ART_MODULE(mu2e::ExtMonFNAL::EMFDetHistPatRec);
