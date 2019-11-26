//Author: S Middleton
//Date: Nov 2019
//Purpose: New Digi product
#ifndef RecoDataProducts_NewCaloDigi_hh
#define RecoDataProducts_NewCaloDigi_hh

#include <vector>

namespace mu2e
{

  class NewCaloDigi {

      public:

	  NewCaloDigi(): _roId(-1), _t0(0.), _waveform(0), _peakpos(0.), _errorFlag(0), _eventMode(0) {}

	  NewCaloDigi(int ROId, int t0, std::vector<int>& waveform, double peakpos, uint16_t errorFlag, uint8_t eventMode):
	    _roId(ROId),
	    _t0(t0),
	    _waveform(waveform),
	    _peakpos(peakpos),
	    _errorFlag(errorFlag),
	    _eventMode(eventMode)
	  {}

	  int                     roId()      const { return _roId;}    
	  int                     t0()        const { return _t0;}
	  const std::vector<int>& waveform()  const { return _waveform; }
	  float 		  peakpos()   const { return _peakpos;	}
	  uint16_t 		  errorFlag() const { return _errorFlag; }
	  uint8_t 		  eventMode() const { return _eventMode; }

      private:

	  int               _roId;      
	  int               _t0;        //time of the first digitezd bin of the signal
	  std::vector<int>  _waveform;  //array of the samples associated with the digitezed signal
	  float    	    _peakpos;	//peak position	for fast estimate of total charge and hit time
	  uint16_t	    _errorFlag; //flag for errors
	  uint8_t           _eventMode; //gives info on event mode
  };

 
   typedef std::vector<mu2e::NewCaloDigi> NewCaloDigiCollection;

}

#endif
