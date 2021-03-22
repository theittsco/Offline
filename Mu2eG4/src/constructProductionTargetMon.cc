//
// Free function. Approach borrowed from constructPS
// Constructs the downstream production target scanning monitor.
// Parent volume is the air in the target hall. Probably?
//

// C++ includes
#include <iostream>
#include <vector>
#include <string>

// Mu2e includes
#include "G4Helper/inc/VolumeInfo.hh"
#include "Mu2eG4/inc/nestBox.hh"
#include "ConfigTools/inc/SimpleConfig.hh"
#include "G4Helper/inc/AntiLeakRegistry.hh"
#include "G4Helper/inc/G4Helper.hh"
#include "Mu2eG4/inc/findMaterialOrThrow.hh"
#include "Mu2eG4/inc/finishNesting.hh"

// G4 inludes
#include "G4Material.hh"
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4RotationMatrix.hh"
#include "G4Types.hh"
#include "G4ThreeVector.hh"
#include "G4Colour.hh"
#include "G4SubtractionSolid.hh"

#include "CLHEP/Units/SystemOfUnits.h"


using namespace std;

namespace mu2e {

    // TODO: add more structure
    // TODO: run mu2e -c Mu2eG4/fcl/gdmldump.fcl

    void constructTargetHallPWC(VolumeInfo const & parent, SimpleConfig const & _config, std::string nameSuffix, G4ThreeVector positionInParent) {
        double gasLength = _config.getDouble("pTargetMon_gasLength");
        double outerPlateLength = _config.getDouble("pTargetMon_outerPlateLength");
        double windowWidth = _config.getDouble("pTargetMon_windowWidth");
        double windowHeight = _config.getDouble("pTargetMon_windowHeight");
        double height = _config.getDouble("pTargetMon_height");
        double width = _config.getDouble("pTargetMon_width");
        double windowThick = _config.getDouble("pTargetMon_windowThick");
        double frameThick = _config.getDouble("pTargetMon_frameThick");
        double wireSpacing = _config.getDouble("pTargetMon_wireSpacing");
        double detectorLength = gasLength + (2*outerPlateLength) + frameThick;


        std::vector<double> halfDims;
        halfDims.push_back(width/2.);
        halfDims.push_back(height/2.);
        halfDims.push_back(detectorLength/2.);

        AntiLeakRegistry& reg = art::ServiceHandle<G4Helper>()->antiLeakRegistry();
        G4RotationMatrix* noRotation = reg.add(new G4RotationMatrix);

        G4Material* baseMaterial = parent.logical->GetMaterial();

        // "container": box representing the location of the individual PWC

        std::string containerName = "PWCContainer";
        containerName.append(nameSuffix);
        VolumeInfo PWCContainerInfo = nestBox(containerName,
                            halfDims,
                            baseMaterial,
                            noRotation,
                            positionInParent,
                            parent,
                            0,
                            G4Colour::Green(),
                            "PTM");

        
        // G10 frame of the PWC, represented as one piece here
        G4Box *outerBox = new G4Box("pwcFrameOuter", 
                                height/2., 
                                width/2., 
                                detectorLength/2.);
        G4Box *innerBox = new G4Box("pwcFrameInner", 
                                0.001+windowHeight/2., 
                                0.001+windowWidth/2., 
                                0.001+detectorLength/2.);
        std::string frameName = "pTargetMonFrame";
        frameName.append(nameSuffix);
        G4Material *frameMaterial = findMaterialOrThrow(_config.getString("pTargetMon_frameMaterial"));

        VolumeInfo frameInfo;
        frameInfo.name = frameName;
        frameInfo.solid = new G4SubtractionSolid(frameName, outerBox, innerBox);
        finishNesting(frameInfo, 
                    frameMaterial, 
                    noRotation, 
                    G4ThreeVector(0.0, 0.0, 0.0), 
                    PWCContainerInfo.logical, 
                    0, 
                    G4Colour::Blue(), 
                    "PTM");

        // insert the windows
        G4Material *windowMaterial = findMaterialOrThrow(_config.getString("pTargetMon_windowMaterial"));
        std::vector<double> windowHalfDims;
        windowHalfDims.push_back(windowWidth/2.);
        windowHalfDims.push_back(windowHeight/2.);
        windowHalfDims.push_back(windowThick/2.);
        // first ground plane
        // TODO: make this a virtual detector
        std::string ground1Name = "pTargetMonGround1";
        ground1Name.append(nameSuffix);
        double ground1Z = -5.5*frameThick;
        nestBox(ground1Name,
                windowHalfDims,
                windowMaterial,
                noRotation,
                G4ThreeVector(0.0, 0.0, ground1Z),
                PWCContainerInfo,
                0,
                G4Colour::Green(),
                "PTM");
        // first HV plane
        std::string hv1Name = "pTargetMonHV1";
        hv1Name.append(nameSuffix);
        double hv1Z = -3.5*frameThick;
        nestBox(hv1Name,
                windowHalfDims,
                windowMaterial,
                noRotation,
                G4ThreeVector(0.0, 0.0, hv1Z),
                PWCContainerInfo,
                0,
                G4Colour::Green(),
                "PTM");
        // second HV plane
        std::string hv2Name = "pTargetMonHV2";
        hv2Name.append(nameSuffix);
        double hv2Z = 0.5*frameThick;
        nestBox(hv2Name,
                windowHalfDims,
                windowMaterial,
                noRotation,
                G4ThreeVector(0.0, 0.0, hv2Z),
                PWCContainerInfo,
                0,
                G4Colour::Green(),
                "PTM");
        // third HV plane
        std::string hv3Name = "pTargetMonHV3";
        hv3Name.append(nameSuffix);
        double hv3Z = 4.5*frameThick;
        nestBox(hv3Name,
                windowHalfDims,
                windowMaterial,
                noRotation,
                G4ThreeVector(0.0, 0.0, hv3Z),
                PWCContainerInfo,
                0,
                G4Colour::Green(),
                "PTM");
        // last ground plane
        std::string ground2Name = "pTargetMonGround2";
        ground2Name.append(nameSuffix);
        double ground2Z = 6.5*frameThick;
        nestBox(ground2Name,
                windowHalfDims,
                windowMaterial,
                noRotation,
                G4ThreeVector(0.0, 0.0, ground2Z),
                PWCContainerInfo,
                0,
                G4Colour::Green(),
                "PTM");

        
        // gas inside PWC
        G4Material *gasMaterial = findMaterialOrThrow(_config.getString("pTargetMon_innerGas"));
        // between ground plane 1 and HV plane 1
        std::string gasName1 = "pTargetMonGas1";
        gasName1.append(nameSuffix);
        double gasLength1 = hv1Z - ground1Z - windowThick;
        double gasZ1 = 0.5*(hv1Z + ground1Z);
        std::vector<double> gasHalfDims1;
        gasHalfDims1.push_back(windowWidth/2.);
        gasHalfDims1.push_back(windowHeight/2.);
        gasHalfDims1.push_back(gasLength1/2.);
        nestBox(gasName1,
                gasHalfDims1,
                gasMaterial,
                noRotation,
                G4ThreeVector(0.0, 0.0, gasZ1),
                PWCContainerInfo,
                0,
                G4Colour::Red(),
                "PTM");
        // between HV plane 1 and HV plane 2
        // TODO: treat as indiv sensitive detectors
        std::string wireGasNameVert = "pTargetMonWireVert";
        wireGasNameVert.append(nameSuffix);
        double gasLength2 = hv2Z - hv1Z - windowThick;
        double gasZ2 = 0.5*(hv2Z + hv1Z);
        int numVertWires = static_cast<int>(windowHeight/wireSpacing);
        for (int i=0; i<numVertWires; i++) {
            std::string wireGasName = wireGasNameVert;
            wireGasName.append(std::to_string(i));
            std::vector<double> gasHalfDims2;
            gasHalfDims2.push_back(windowWidth/2.);
            gasHalfDims2.push_back(wireSpacing/2.);
            gasHalfDims2.push_back(gasLength2/2.);
            double gasY2 = (-0.5*windowHeight) + ((i+0.5)*wireSpacing);
            nestBox(wireGasName,
                gasHalfDims2,
                gasMaterial,
                noRotation,
                G4ThreeVector(0.0, gasY2, gasZ2),
                PWCContainerInfo,
                0,
                G4Colour::Red(),
                "PTM");
        }
        
        // TODO: treat as indiv sensitive detectors
        std::string wireGasNameHoriz = "pTargetMonWireHoriz";
        wireGasNameHoriz.append(nameSuffix);
        double gasLength3 = hv3Z - hv2Z - windowThick;
        double gasZ3 = 0.5*(hv3Z + hv2Z);
        int numHorizWires = static_cast<int>(windowWidth/wireSpacing);
        for (int i=0; i<numHorizWires; i++) {
            std::string wireGasName = wireGasNameHoriz;
            wireGasName.append(std::to_string(i));
            std::vector<double> gasHalfDims3;
            gasHalfDims3.push_back(wireSpacing/2.);
            gasHalfDims3.push_back(windowHeight/2.);
            gasHalfDims3.push_back(gasLength3/2.);
            double gasX3 = (-0.5*windowWidth) + ((i+0.5)*wireSpacing);
            nestBox(wireGasName,
                gasHalfDims3,
                gasMaterial,
                noRotation,
                G4ThreeVector(gasX3, 0.0, gasZ3),
                PWCContainerInfo,
                0,
                G4Colour::Red(),
                "PTM");
        }

        // between HV plane 3 and ground plane 2
        std::string gasName4 = "pTargetMonGas4";
        gasName4.append(nameSuffix);
        double gasLength4 = ground2Z - hv3Z - windowThick;
        double gasZ4 = 0.5*(ground2Z + hv3Z);
        std::vector<double> gasHalfDims4;
        gasHalfDims4.push_back(windowWidth/2.);
        gasHalfDims4.push_back(windowHeight/2.);
        gasHalfDims4.push_back(gasLength4/2.);
        nestBox(gasName4,
                gasHalfDims4,
                gasMaterial,
                noRotation,
                G4ThreeVector(0.0, 0.0, gasZ4),
                PWCContainerInfo,
                0,
                G4Colour::Red(),
                "PTM");

        
    } //constructTargetHallPWC


    void constructProductionTargetMon(VolumeInfo const & parent, SimpleConfig const & _config) {
        cout << endl << endl;
        cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
        cout << "Now ENTERING constructProductionTargetMon" << endl;
        cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl << endl;
        // for now, do this in a simple way that hopefully works?
        double height = _config.getDouble("pTargetMon_height");
        double width = _config.getDouble("pTargetMon_width");
        double length = _config.getDouble("pTargetMon_length");
        //double halfDimsRaw[3] = {width/2., height/2., length/2.};
        std::vector<double> halfDims;
        halfDims.push_back(width/2.);
        halfDims.push_back(height/2.);
        halfDims.push_back(length/2.);

        G4double xPosInMu2e = _config.getDouble("pTargetMon_positionX");
        G4double yPosInMu2e = _config.getDouble("pTargetMon_positionY");
        G4double zPosInMu2e = _config.getDouble("pTargetMon_positionZ");
        G4ThreeVector positionInMu2e = G4ThreeVector(xPosInMu2e, yPosInMu2e, zPosInMu2e);

        double yRotInMu2e = _config.getDouble("pTargetMon_rotY")*-1.;
        double xRotInMu2e = _config.getDouble("pTargetMon_rotX")*-1.;
        AntiLeakRegistry& reg = art::ServiceHandle<G4Helper>()->antiLeakRegistry();
        G4RotationMatrix* rotation = reg.add(new G4RotationMatrix);
        //G4RotationMatrix* rotation = new G4RotationMatrix();
        rotation->rotateY(yRotInMu2e*CLHEP::deg);
        rotation->rotateX(xRotInMu2e*CLHEP::deg);

        G4ThreeVector _hallOriginInMu2e = parent.centerInMu2e();

        G4Material* baseMaterial = parent.logical->GetMaterial();

        // container: holds the 2 actual detectors
        VolumeInfo pTargetMonContainer = nestBox("pTargetMonContainer",
                                halfDims,
                                baseMaterial,
                                rotation,
                                positionInMu2e-_hallOriginInMu2e,
                                parent,
                                0,
                                G4Colour::Green(),
                                "PTM");

        double gasLength = _config.getDouble("pTargetMon_gasLength");
        double outerPlateLength = _config.getDouble("pTargetMon_outerPlateLength");
        double detectorLength = gasLength + (2*outerPlateLength);

        // location of one PWC:
        double z1 = (-0.5*length)+(detectorLength/2.);
        G4ThreeVector position_1 = G4ThreeVector(0.0, 0.0, z1);
        constructTargetHallPWC(pTargetMonContainer, _config, "_1", position_1);
        // second PWC:
        double z2 = (0.5*length)-(detectorLength/2.);
        G4ThreeVector position_2 = G4ThreeVector(0.0, 0.0, z2);
        constructTargetHallPWC(pTargetMonContainer, _config, "_2", position_2);


        cout << endl << endl;
        cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl;
        cout << "Now LEAVING constructProductionTargetMon" << endl;
        cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl << endl;

    } // constructProductionTargetMon


    

    
} // namespace mu2e