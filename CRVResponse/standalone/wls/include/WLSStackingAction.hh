#ifndef WLSStackingAction_h
#define WLSStackingAction_h 1

#include "globals.hh"
#include "G4UserStackingAction.hh"
#include "G4Track.hh"
#include "G4VProcess.hh"
#include <map>

class WLSStackingAction : public G4UserStackingAction
{

  private:

    static WLSStackingAction*  _fgInstance;
    int    _scintillation, _cerenkovS, _cerenkovF;
    int    _totalScintillation, _totalCerenkov;

  public:

    WLSStackingAction() 
    {
      _fgInstance = this;
      _totalScintillation=0;
      _totalCerenkov=0;
    }
    virtual ~WLSStackingAction() {}

    void PrepareNewEvent() 
    {
      _scintillation=0;
      _cerenkovS=0;
      _cerenkovF=0;
    }

    static  WLSStackingAction* Instance() {return _fgInstance;}
    virtual G4ClassificationOfNewTrack ClassifyNewTrack(const G4Track* track)
    {
      if(track->GetDefinition()->GetPDGEncoding()==0 && track->GetVolume() && track->GetCreatorProcess())
      {
        std::string v=track->GetVolume()->GetName();
        std::string p=track->GetCreatorProcess()->GetProcessName();
        if(v=="Scintillator" && p=="Scintillation") _scintillation++;
        if(v=="Scintillator" && p=="CerenkovNew") _cerenkovS++;
//        if(v=="Scintillator" && p=="Scintillation") {_scintillation++; return fKill;}
//        if(v=="Scintillator" && p=="CerenkovNew") {_cerenkovS++; return fKill;}
        if((v=="WLSFiber" || v.compare(0,4,"Clad")==0) && p=="CerenkovNew") _cerenkovF++;
      }
      return fUrgent;
    }

    void PrintStatus()
    {
      std::cout<<"Full GEANT4:  Scintillation Photons: "<<_scintillation<<"    Cerenkov Photons (scintillator): "<<_cerenkovS<<"    Cerenkov Photons (fiber): "<<_cerenkovF<<std::endl;
      _totalScintillation+=_scintillation;
      _totalCerenkov+=_cerenkovS+_cerenkovF;
      std::cout<<"Full GEANT4:  total scintillation: "<<_totalScintillation<<"  total Cerenkov: "<<_totalCerenkov<<std::endl;
    }
};

#endif

