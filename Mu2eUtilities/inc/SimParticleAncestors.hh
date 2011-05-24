#ifndef Mu2eUtilities_SimParticleAncestors_hh
#define Mu2eUtilities_SimParticleAncestors_hh
//
// Start with a SimParticle and trace its ancestry back to a generated particle.
//
// $Id: SimParticleAncestors.hh,v 1.6 2011/05/24 17:19:03 kutschke Exp $
// $Author: kutschke $
// $Date: 2011/05/24 17:19:03 $
//
// Original author Rob Kutschke
//
// Notes:
// 1) This class is not designed to be persisted because it has bare pointers.
//

#include "MCDataProducts/inc/GenParticleCollection.hh"
#include "MCDataProducts/inc/SimParticleCollection.hh"

namespace mu2e{


  class SimParticleAncestors{

  public:
    typedef SimParticleCollection::key_type key_type;

    SimParticleAncestors( key_type key,
                          SimParticleCollection const& sims,
                          GenParticleCollection const & gens,
                          int maxDepth=100);

    SimParticleAncestors( SimParticle const& sim,
                          SimParticleCollection const& sims,
                          GenParticleCollection const & gens,
                          int maxDepth=100);

    // Compiler generated code is Ok for:
    //  d'tor, copy c'tor assignment operator.

    SimParticle    const& sim()         const {return *_sim;}
    SimParticle    const& originalSim() const {return *_sim0;}
    GenParticle const& originalGen() const {return *_gen0;}
    int                   depth()       const {return _depth;}

  private:

    // Non-owning pointers to the input particle.
    SimParticle    const* _sim;

    // Non-owning pointers the generated particle that is in the ancestry list
    // of the input particle; also the sim particle that comes
    // directly from the generated particle.
    SimParticle    const* _sim0;
    GenParticle const* _gen0;

    // Number of generations from the generated particle to this one.
    int _depth;

    // Limit on number of generations in search for the generated particle.
    int _maxDepth;

    // A helper function to do the work that is common to the several constructors.
    void construct( SimParticleCollection const& sims,
                    GenParticleCollection const & gens);
  };

} // end namespace mu2e


#endif /* Mu2eUtilities_SimParticleAncestors_hh */
