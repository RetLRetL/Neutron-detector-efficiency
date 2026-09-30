#include "detector.h"
#include "settings.h"
#include "he3_electric_field.h"

#include "G4Box.hh"
#include "G4Colour.hh"
#include "G4Element.hh"
#include "G4FieldManager.hh"
#include "G4IntegrationDriver.hh"
#include "G4Isotope.hh"
#include "G4LogicalVolume.hh"
#include "G4Material.hh"
#include "G4NistManager.hh"
#include "G4PVPlacement.hh"
#include "G4SubtractionSolid.hh"
#include "G4SystemOfUnits.hh"
#include "G4Tubs.hh"
#include "G4VisAttributes.hh"
#include "G4EqMagElectricField.hh"
#include "G4ClassicalRK4.hh"
#include "G4ChordFinder.hh"
#include "G4PhysicalConstants.hh"

DetectorConstruction::DetectorConstruction(
    const SimulationConfig& config
)
    : fConfig(config)
{
}

G4VPhysicalVolume* DetectorConstruction::Construct()
{
    G4NistManager* nist = G4NistManager::Instance();

    // ================================================================
    // Materials
    // ================================================================

    G4Material* air =
        nist->FindOrBuildMaterial("G4_AIR");

    G4Material* copper =
        nist->FindOrBuildMaterial("G4_Cu");

    G4Material* polyethylene =
        nist->FindOrBuildMaterial("G4_POLYETHYLENE");

    G4Material* steel =
        new G4Material(
            "Steel",
            7.87 * g / cm3,
            2
        );

    steel->AddElement(
        nist->FindOrBuildElement("Fe"),
        0.998
    );

    steel->AddElement(
        nist->FindOrBuildElement("C"),
        0.002
    );

    // ================================================================
    // He-3 gas
    // ================================================================

    G4Isotope* He3 =
        new G4Isotope(
            "He3",
            2,
            3,
            3.016 * g / mole
        );

    G4Element* helium3 =
        new G4Element(
            "Helium3",
            "He3",
            1
        );

    helium3->AddIsotope(
        He3,
        100.0 * perCent
    );

    const G4double density =
        Settings::GAS_PRESSURE *
        (3.016 * g / mole) /
        (
            Avogadro *
            k_Boltzmann *
            Settings::GAS_TEMPERATURE
        );

    G4Material* helium3Gas =
        new G4Material(
            "He3Gas",
            density,
            1,
            kStateGas,
            Settings::GAS_TEMPERATURE,
            Settings::GAS_PRESSURE
        );

    helium3Gas->AddElement(
        helium3,
        1.0
    );

    // ================================================================
    // Detector envelope
    //
    // IMPORTANT:
    // The envelope length along X is now equal to the steel tube length.
    // Therefore:
    //
    //     L_envelope_X = L_steel = 380 mm
    //
    // ================================================================

    const G4double envelopeLengthX =
        Settings::STEEL_LENGTH;

    G4Box* envelopeSolid =
        new G4Box(
            "Envelope",
            envelopeLengthX / 2.0,
            Settings::ENVELOPE_SIZE_Y / 2.0,
            Settings::ENVELOPE_SIZE_Z / 2.0
        );

    // ================================================================
    // World
    // ================================================================

    G4Box* worldSolid =
        new G4Box(
            "World",
            Settings::WORLD_HALF_X,
            Settings::WORLD_HALF_Y,
            Settings::WORLD_HALF_Z 
        );

    G4LogicalVolume* worldLogic =
        new G4LogicalVolume(
            worldSolid,
            air,
            "World"
        );

    G4VPhysicalVolume* worldPhysical =
        new G4PVPlacement(
            nullptr,
            G4ThreeVector(),
            worldLogic,
            "World",
            nullptr,
            false,
            0,
            true
        );

    G4LogicalVolume* envelopeLogic =
        new G4LogicalVolume(
            envelopeSolid,
            air,
            "Envelope"
        );

    new G4PVPlacement(
        nullptr,
        G4ThreeVector(),
        envelopeLogic,
        "Envelope",
        worldLogic,
        false,
        0,
        true
    );

    // ================================================================
    // Detector axis
    //
    // G4Tubs is initially along local Z.
    // Rotate it by 90 degrees around Y:
    // local Z -> global X.
    // ================================================================

    G4RotationMatrix* detectorRotation =
        new G4RotationMatrix();

    detectorRotation->rotateY(
        90.0 * deg
    );

    // ================================================================
    // Steel tube
    //
    // Full length = 380 mm
    // Wall thickness = 1 mm
    //
    // Along X:
    //     -190 ... +190 mm
    //
    // ================================================================

    G4Tubs* steelSolid =
        new G4Tubs(
            "SteelTube",
            Settings::STEEL_INNER_RADIUS,
            Settings::STEEL_OUTER_RADIUS,
            Settings::STEEL_LENGTH / 2.0,
            0.0,
            360.0 * deg
        );

    G4LogicalVolume* steelLogic =
        new G4LogicalVolume(
            steelSolid,
            steel,
            "SteelTube"
        );

    new G4PVPlacement(
        detectorRotation,
        G4ThreeVector(),
        steelLogic,
        "SteelTube",
        envelopeLogic,
        false,
        0,
        true
    );

    // ================================================================
    // He-3 gas
    //
    // Previous half-length:
    //     188 mm
    //
    // New half-length:
    //     188 + 1 = 189 mm
    //
    // Therefore full gas length = 378 mm.
    //
    // Along X:
    //     -189 ... +189 mm
    //
    // ================================================================

    const G4double gasHalfLength =
        Settings::GAS_HALF_LENGTH +
        Settings::STEEL_WALL_THICKNESS;

    G4Tubs* gasSolid =
        new G4Tubs(
            "He3Cylinder",
            0.0,
            Settings::GAS_RADIUS,
            gasHalfLength,
            0.0,
            360.0 * deg
        );

    // ================================================================
    // Copper wire
    //
    // Wire has exactly the same final half-length as gas.
    //
    // ================================================================

    const G4double wireHalfLength =
        Settings::WIRE_HALF_LENGTH +
        Settings::STEEL_WALL_THICKNESS;

    G4Tubs* wireCutSolid =
        new G4Tubs(
            "WireCut",
            0.0,
            Settings::WIRE_RADIUS,
            wireHalfLength,
            0.0,
            360.0 * deg
        );

    G4SubtractionSolid* gasMinusWire =
        new G4SubtractionSolid(
            "He3MinusWire",
            gasSolid,
            wireCutSolid
        );

    G4LogicalVolume* gasLogic =
        new G4LogicalVolume(
            gasMinusWire,
            helium3Gas,
            "He3Gas"
        );

    // ================================================================
    // Steel end disks
    //
    // Gas now ends at:
    //
    //     X = ±189 mm
    //
    // Each disk has thickness 1 mm.
    //
    // Therefore disk centers are at:
    //
    //     X = ±189.5 mm
    //
    // and the disks occupy:
    //
    //     [-190, -189] mm
    //     [+189, +190] mm
    //
    // This exactly closes the steel tube.
    // ================================================================

    G4Tubs* capSolid =
        new G4Tubs(
            "SteelEndCap",
            0.0,
            Settings::STEEL_INNER_RADIUS,
            Settings::STEEL_WALL_THICKNESS / 2.0,
            0.0,
            360.0 * deg
        );

    G4LogicalVolume* capLogic =
        new G4LogicalVolume(
            capSolid,
            steel,
            "SteelEndCap"
        );

    const G4double capCenterX =
        gasHalfLength +
        Settings::STEEL_WALL_THICKNESS / 2.0;

    new G4PVPlacement(
        detectorRotation,
        G4ThreeVector(
            -capCenterX,
            0.0,
            0.0
        ),
        capLogic,
        "SteelEndCapMinus",
        envelopeLogic,
        false,
        0,
        true
    );

    new G4PVPlacement(
        detectorRotation,
        G4ThreeVector(
            +capCenterX,
            0.0,
            0.0
        ),
        capLogic,
        "SteelEndCapPlus",
        envelopeLogic,
        false,
        1,
        true
    );

    // ================================================================
    // Copper wire itself
    // ================================================================

    G4Tubs* wireSolid =
        new G4Tubs(
            "Wire",
            0.0,
            Settings::WIRE_RADIUS,
            wireHalfLength,
            0.0,
            360.0 * deg
        );

    G4LogicalVolume* wireLogic =
        new G4LogicalVolume(
            wireSolid,
            copper,
            "Wire"
        );

    new G4PVPlacement(
        detectorRotation,
        G4ThreeVector(),
        wireLogic,
        "Wire",
        envelopeLogic,
        false,
        0,
        true
    );

    // ================================================================
    // Polyethylene
    // ================================================================
    //
    // PE thickness T_PE is the thickness outside the detector envelope.
    //
    // Therefore:
    //
    //     PE_HALF_Y = T_PE + 25 mm
    //     PE_HALF_Z = T_PE + 25 mm
    //
    // Along X:
    //
    //     envelope half-length = 190 mm
    //     extra PE half-width = 10 mm
    //
    //     PE_HALF_X = 200 mm
    //
    // PE is shifted by 11 mm along +X.
    //
    // Thus after placement:
    //
    //     PE X range = [-189, +211] mm
    //
    // while envelope is:
    //
    //     Envelope X range = [-190, +190] mm
    //
    // Therefore a 1 mm opening appears at the -X end.
    //
    // ================================================================

    const G4double PE_SHIFT_X =
        Settings::PE_SHIFT_X +
        Settings::STEEL_WALL_THICKNESS;

    const G4double PE_HALF_X =
        envelopeLengthX / 2.0 +
        10.0 * mm;

    const G4double PE_HALF_Y =
        fConfig.peThickness +
        Settings::ENVELOPE_SIZE_Y / 2.0;

    const G4double PE_HALF_Z =
        fConfig.peThickness +
        Settings::ENVELOPE_SIZE_Z / 2.0;

    G4Box* polyethyleneOuterSolid =
        new G4Box(
            "PolyethyleneOuter",
            PE_HALF_X,
            PE_HALF_Y,
            PE_HALF_Z
        );

    // ================================================================
    // Subtract envelope
    //
    // PE will later be shifted by +11 mm.
    // Therefore the cavity has to be at local X = -11 mm.
    //
    // After physical placement:
    //
    //     -11 + 11 = 0
    //
    // so the envelope cavity is centered at global X = 0.
    // ================================================================

    G4SubtractionSolid* polyethyleneWithEnvelopeCavity =
        new G4SubtractionSolid(
            "PolyethyleneWithEnvelopeCavity",
            polyethyleneOuterSolid,
            envelopeSolid,
            nullptr,
            G4ThreeVector(
                -PE_SHIFT_X,
                0.0,
                0.0
            )
        );

    // ================================================================
    // Cf-252 sample
    // ================================================================

    G4Isotope* cf252iso =
        new G4Isotope(
            "Cf252",
            98,
            252,
            252.08 * g / mole
        );

    G4Element* cfEl =
        new G4Element(
            "Californium",
            "Cf",
            1
        );

    cfEl->AddIsotope(
        cf252iso,
        100.0 * perCent
    );

    G4Material* cfMaterial =
        new G4Material(
            "Cf252",
            15.1 * g / cm3,
            1
        );

    cfMaterial->AddElement(
        cfEl,
        1.0
    );

    // Sample:
    //
    // radius = 20 mm
    // height = 10 mm
    //
    // Its axis is global Y.
    // ================================================================

    G4Tubs* sampleSolid =
        new G4Tubs(
            "Cf252Sample",
            0.0,
            Settings::SAMPLE_RADIUS,
            Settings::SAMPLE_HEIGHT / 2.0,
            0.0,
            360.0 * deg
        );

    G4RotationMatrix* sampleRotation =
        new G4RotationMatrix();

    sampleRotation->rotateX(
        90.0 * deg
    );

    G4LogicalVolume* sampleLogic =
        new G4LogicalVolume(
            sampleSolid,
            cfMaterial,
            "Cf252Sample"
        );

    // ================================================================
    // Sample position
    //
    // Shift along X by half the detector length:
    //
    //     X = 190 mm
    //
    // Along Y:
    //
    //     Y = T_PE
    //
    // Therefore the sample occupies:
    //
    //     Y = T_PE - 5 ... T_PE + 5 mm
    //
    // ================================================================

    const G4double sampleY =
        fConfig.peThickness +
        Settings::ENVELOPE_SIZE_Y / 2.0;

    const G4ThreeVector samplePosition(
        0.0,
        sampleY,
        0.0
    );

    // Convert global sample position to local PE coordinates.
    const G4ThreeVector samplePositionInPE =
        samplePosition -
        G4ThreeVector(
            PE_SHIFT_X,
            0.0,
            0.0
        );

    // ================================================================
    // Subtract sample volume from PE
    // ================================================================

    G4SubtractionSolid* polyethyleneFinalSolid =
        new G4SubtractionSolid(
            "PolyethyleneFinal",
            polyethyleneWithEnvelopeCavity,
            sampleSolid,
            sampleRotation,
            samplePositionInPE
        );

    G4LogicalVolume* polyethyleneLogic =
        new G4LogicalVolume(
            polyethyleneFinalSolid,
            polyethylene,
            "Polyethylene"
        );

    // ================================================================
    // Place PE
    // ================================================================

    new G4PVPlacement(
        nullptr,
        G4ThreeVector(
            PE_SHIFT_X,
            0.0,
            0.0
        ),
        polyethyleneLogic,
        "Polyethylene",
        worldLogic,
        false,
        0,
        true
    );

    // ================================================================
    // Place actual Cf-252 sample
    // ================================================================

    new G4PVPlacement(
        sampleRotation,
        samplePosition,
        sampleLogic,
        "Cf252Sample",
        worldLogic,
        false,
        0,
        true
    );

    // ================================================================
    // Electric field
    // ================================================================

    He3ElectricField* field =
        new He3ElectricField();

    G4EqMagElectricField* equation =
        new G4EqMagElectricField(
            field
        );

    constexpr G4int nvar = 8;

    G4ClassicalRK4* stepper =
        new G4ClassicalRK4(
            equation,
            nvar
        );

    G4IntegrationDriver<G4ClassicalRK4>* driver =
        new G4IntegrationDriver<G4ClassicalRK4>(
            Settings::FIELD_MIN_STEP,
            stepper,
            nvar
        );

    G4ChordFinder* chordFinder =
        new G4ChordFinder(
            driver
        );

    G4FieldManager* fieldManager =
        new G4FieldManager(
            field,
            chordFinder
        );

    gasLogic->SetFieldManager(
        fieldManager,
        true
    );

    // ================================================================
    // Place He-3 gas
    // ================================================================

    new G4PVPlacement(
        detectorRotation,
        G4ThreeVector(),
        gasLogic,
        "He3Gas",
        envelopeLogic,
        false,
        0,
        true
    );

    // ================================================================
    // Visualization
    // ================================================================

    G4VisAttributes* envelopeVis =
        new G4VisAttributes();

    envelopeVis->SetVisibility(false);

    envelopeLogic->SetVisAttributes(
        envelopeVis
    );

    G4VisAttributes* steelVis =
        new G4VisAttributes(
            G4Colour(
                0.7,
                0.7,
                0.7
            )
        );

    steelVis->SetVisibility(true);
    steelVis->SetForceSolid(true);

    steelLogic->SetVisAttributes(
        steelVis
    );

    capLogic->SetVisAttributes(
        steelVis
    );

    G4VisAttributes* heliumVis =
        new G4VisAttributes(
            G4Colour(
                0.2,
                0.8,
                1.0
            )
        );

    heliumVis->SetVisibility(true);
    heliumVis->SetForceSolid(true);

    gasLogic->SetVisAttributes(
        heliumVis
    );

    G4VisAttributes* wireVis =
        new G4VisAttributes(
            G4Colour(
                0.9,
                0.5,
                0.1
            )
        );

    wireVis->SetVisibility(true);
    wireVis->SetForceSolid(true);

    wireLogic->SetVisAttributes(
        wireVis
    );

    G4VisAttributes* polyethyleneVis =
        new G4VisAttributes(
            G4Colour(
                0.8,
                0.8,
                0.2
            )
        );

    polyethyleneVis->SetVisibility(true);
    polyethyleneVis->SetForceSolid(true);

    polyethyleneLogic->SetVisAttributes(
        polyethyleneVis
    );

    G4VisAttributes* sampleVis = 
        new G4VisAttributes(
            G4Colour(
                0.9,
                0.1,
                0.1
            )
        );

    sampleVis->SetVisibility(true);
    sampleVis->SetForceSolid(true);

    sampleLogic->SetVisAttributes(
        sampleVis
    );

    return worldPhysical;
}