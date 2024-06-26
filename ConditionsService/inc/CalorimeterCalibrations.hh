#ifndef ConditionsService_CalorimeterCalibrations_hh
#define ConditionsService_CalorimeterCalibrations_hh


// C++ includes.
#include <iostream>
#include <string>

// Mu2e includes.
#include "Offline/Mu2eInterfaces/inc/ConditionsEntity.hh"
#include "CLHEP/Vector/ThreeVector.h"


namespace mu2e
{

  class SimpleConfig;


  class CalorimeterCalibrations: public ConditionsEntity {

  public:

    explicit CalorimeterCalibrations ( SimpleConfig const& config );
    ~CalorimeterCalibrations(){};

    const std::string& pulseFileName()  const {return _pulseFileName;}
    const std::string& pulseHistName()  const {return _pulseHistName;}
    const std::string& propagFileName() const {return _propagFileName;}
    const std::string& propagHistName() const {return _propagHistName;}

    double BirkCorrHadron()            const {return _BirkCorrHadron;}
    double LRUpar0(int crystalId)      const {return _LRUpar0;}
    double peMeV(int crystalId)        const {return _peMeV;}
    double ROnoise(int roId)           const {return _ROnoise;}
    double ADC2MeV(int roId)           const {return _ADC2MeV;}
    double MeV2ADC(int roId)           const {return 1.0/_ADC2MeV;}
    double Peak2MeV(int roId)          const {return _Peak2MeV;}

  private:

    std::string  _pulseFileName;
    std::string  _pulseHistName;
    std::string  _propagFileName;
    std::string  _propagHistName;

    double       _LRUpar0;
    double       _BirkCorrHadron;
    double       _peMeV;
    double       _ROnoise;
    double       _ADC2MeV;
    double          _Peak2MeV;
  };


}

#endif /* ConditionsService_CalorimeterCalibrations_hh */
