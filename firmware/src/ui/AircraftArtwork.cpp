#include "AircraftArtwork.h"

#include <stdint.h>

#include "aircraft_business_jet.inc"
#include "aircraft_airbus_a350.inc"
#include "aircraft_airbus_a380.inc"
#include "aircraft_atr.inc"
#include "aircraft_beech_baron.inc"
#include "aircraft_beech_bonanza.inc"
#include "aircraft_beech_king_air.inc"
#include "aircraft_beech_vtail_bonanza.inc"
#include "aircraft_boeing_747.inc"
#include "aircraft_boeing_777.inc"
#include "aircraft_boeing_787.inc"
#include "aircraft_cessna.inc"
#include "aircraft_citation.inc"
#include "aircraft_crj.inc"
#include "aircraft_dash8.inc"
#include "aircraft_dhc2_beaver.inc"
#include "aircraft_dhc6_twin_otter.inc"
#include "aircraft_embraer_ejet.inc"
#include "aircraft_gulfstream.inc"
#include "aircraft_helicopter.inc"
#include "aircraft_high_performance.inc"
#include "aircraft_light_aircraft.inc"
#include "aircraft_long_narrowbody.inc"
#include "aircraft_narrowbody.inc"
#include "aircraft_piper_cherokee.inc"
#include "aircraft_piper_mclass.inc"
#include "aircraft_piper_seneca.inc"
#include "aircraft_piper_super_cub.inc"
#include "aircraft_regional_jet.inc"
#include "aircraft_turboprop.inc"
#include "aircraft_widebody.inc"

namespace
{
    const lv_img_dsc_t NARROW_BODY_IMAGE = {
        {LV_IMG_CF_RAW_ALPHA, 0, 0, 280, 149},
        aircraft_narrowbody_png_len, aircraft_narrowbody_png};
    const lv_img_dsc_t LONG_NARROW_BODY_IMAGE = {
        {LV_IMG_CF_RAW_ALPHA, 0, 0, 280, 102},
        aircraft_long_narrowbody_png_len, aircraft_long_narrowbody_png};
    const lv_img_dsc_t WIDE_BODY_IMAGE = {
        {LV_IMG_CF_RAW_ALPHA, 0, 0, 280, 149},
        aircraft_widebody_png_len, aircraft_widebody_png};
    const lv_img_dsc_t REGIONAL_JET_IMAGE = {
        {LV_IMG_CF_RAW_ALPHA, 0, 0, 280, 140},
        aircraft_regional_jet_png_len, aircraft_regional_jet_png};
    const lv_img_dsc_t BUSINESS_JET_IMAGE = {
        {LV_IMG_CF_RAW_ALPHA, 0, 0, 280, 149},
        aircraft_business_jet_png_len, aircraft_business_jet_png};
    const lv_img_dsc_t TURBOPROP_IMAGE = {
        {LV_IMG_CF_RAW_ALPHA, 0, 0, 280, 140},
        aircraft_turboprop_png_len, aircraft_turboprop_png};
    const lv_img_dsc_t LIGHT_AIRCRAFT_IMAGE = {
        {LV_IMG_CF_RAW_ALPHA, 0, 0, 280, 140},
        aircraft_light_aircraft_png_len, aircraft_light_aircraft_png};
    const lv_img_dsc_t HELICOPTER_IMAGE = {
        {LV_IMG_CF_RAW_ALPHA, 0, 0, 280, 151},
        aircraft_helicopter_png_len, aircraft_helicopter_png};
    const lv_img_dsc_t HIGH_PERFORMANCE_IMAGE = {
        {LV_IMG_CF_RAW_ALPHA, 0, 0, 280, 157},
        aircraft_high_performance_png_len, aircraft_high_performance_png};
    const lv_img_dsc_t BOEING_747_IMAGE = {
        {LV_IMG_CF_RAW_ALPHA, 0, 0, 280, 79},
        aircraft_boeing_747_png_len, aircraft_boeing_747_png};
    const lv_img_dsc_t AIRBUS_A380_IMAGE = {
        {LV_IMG_CF_RAW_ALPHA, 0, 0, 280, 85},
        aircraft_airbus_a380_png_len, aircraft_airbus_a380_png};
    const lv_img_dsc_t BOEING_777_IMAGE = {
        {LV_IMG_CF_RAW_ALPHA, 0, 0, 280, 79},
        aircraft_boeing_777_png_len, aircraft_boeing_777_png};
    const lv_img_dsc_t BOEING_787_IMAGE = {
        {LV_IMG_CF_RAW_ALPHA, 0, 0, 280, 80},
        aircraft_boeing_787_png_len, aircraft_boeing_787_png};
    const lv_img_dsc_t AIRBUS_A350_IMAGE = {
        {LV_IMG_CF_RAW_ALPHA, 0, 0, 280, 77},
        aircraft_airbus_a350_png_len, aircraft_airbus_a350_png};
    const lv_img_dsc_t CRJ_IMAGE = {
        {LV_IMG_CF_RAW_ALPHA, 0, 0, 280, 61},
        aircraft_crj_png_len, aircraft_crj_png};
    const lv_img_dsc_t EMBRAER_EJET_IMAGE = {
        {LV_IMG_CF_RAW_ALPHA, 0, 0, 280, 85},
        aircraft_embraer_ejet_png_len, aircraft_embraer_ejet_png};
    const lv_img_dsc_t ATR_IMAGE = {
        {LV_IMG_CF_RAW_ALPHA, 0, 0, 280, 78},
        aircraft_atr_png_len, aircraft_atr_png};
    const lv_img_dsc_t DASH8_IMAGE = {
        {LV_IMG_CF_RAW_ALPHA, 0, 0, 280, 72},
        aircraft_dash8_png_len, aircraft_dash8_png};
    const lv_img_dsc_t GULFSTREAM_IMAGE = {
        {LV_IMG_CF_RAW_ALPHA, 0, 0, 280, 67},
        aircraft_gulfstream_png_len, aircraft_gulfstream_png};
    const lv_img_dsc_t CITATION_IMAGE = {
        {LV_IMG_CF_RAW_ALPHA, 0, 0, 280, 78},
        aircraft_citation_png_len, aircraft_citation_png};
    const lv_img_dsc_t CESSNA_IMAGE = {
        {LV_IMG_CF_RAW_ALPHA, 0, 0, 280, 96},
        aircraft_cessna_png_len, aircraft_cessna_png};
    const lv_img_dsc_t PIPER_SUPER_CUB_IMAGE = {
        {LV_IMG_CF_RAW_ALPHA, 0, 0, 280, 91},
        aircraft_piper_super_cub_png_len, aircraft_piper_super_cub_png};
    const lv_img_dsc_t PIPER_CHEROKEE_IMAGE = {
        {LV_IMG_CF_RAW_ALPHA, 0, 0, 280, 102},
        aircraft_piper_cherokee_png_len, aircraft_piper_cherokee_png};
    const lv_img_dsc_t PIPER_SENECA_IMAGE = {
        {LV_IMG_CF_RAW_ALPHA, 0, 0, 280, 103},
        aircraft_piper_seneca_png_len, aircraft_piper_seneca_png};
    const lv_img_dsc_t PIPER_MCLASS_IMAGE = {
        {LV_IMG_CF_RAW_ALPHA, 0, 0, 280, 95},
        aircraft_piper_mclass_png_len, aircraft_piper_mclass_png};
    const lv_img_dsc_t DHC2_BEAVER_IMAGE = {
        {LV_IMG_CF_RAW_ALPHA, 0, 0, 280, 113},
        aircraft_dhc2_beaver_png_len, aircraft_dhc2_beaver_png};
    const lv_img_dsc_t DHC6_TWIN_OTTER_IMAGE = {
        {LV_IMG_CF_RAW_ALPHA, 0, 0, 280, 119},
        aircraft_dhc6_twin_otter_png_len, aircraft_dhc6_twin_otter_png};
    const lv_img_dsc_t BEECH_BONANZA_IMAGE = {
        {LV_IMG_CF_RAW_ALPHA, 0, 0, 280, 91},
        aircraft_beech_bonanza_png_len, aircraft_beech_bonanza_png};
    const lv_img_dsc_t BEECH_VTAIL_BONANZA_IMAGE = {
        {LV_IMG_CF_RAW_ALPHA, 0, 0, 280, 71},
        aircraft_beech_vtail_bonanza_png_len, aircraft_beech_vtail_bonanza_png};
    const lv_img_dsc_t BEECH_BARON_IMAGE = {
        {LV_IMG_CF_RAW_ALPHA, 0, 0, 280, 95},
        aircraft_beech_baron_png_len, aircraft_beech_baron_png};
    const lv_img_dsc_t BEECH_KING_AIR_IMAGE = {
        {LV_IMG_CF_RAW_ALPHA, 0, 0, 280, 98},
        aircraft_beech_king_air_png_len, aircraft_beech_king_air_png};

    bool startsWithAny(const String& value, const char* const* prefixes, size_t count)
    {
        for (size_t index = 0; index < count; ++index)
            if (value.startsWith(prefixes[index])) return true;
        return false;
    }
}

AircraftArtwork::Family AircraftArtwork::classify(
    const String& rawType, const String& rawDescription, const String& rawCategory)
{
    String type = rawType;
    String description = rawDescription;
    String category = rawCategory;
    type.toUpperCase();
    description.toUpperCase();
    category.toUpperCase();

    if (category == "A7" || description.indexOf("HELICOPTER") >= 0 ||
        description.indexOf("ROTORCRAFT") >= 0)
        return Family::Helicopter;
    if (category == "A6" || description.indexOf("FIGHTER") >= 0 ||
        description.indexOf("MILITARY") >= 0)
        return Family::HighPerformance;

    if (type.startsWith("B74")) return Family::Boeing747;
    if (type == "A388") return Family::AirbusA380;
    if (type.startsWith("B77")) return Family::Boeing777;
    if (type.startsWith("B78")) return Family::Boeing787;
    if (type.startsWith("A35")) return Family::AirbusA350;

    if (type == "PA18") return Family::PiperSuperCub;
    if (type.startsWith("PA28") || type.startsWith("P28") ||
        type.startsWith("PA32") || type.startsWith("P32"))
        return Family::PiperCherokee;
    if (type.startsWith("PA34") || type.startsWith("P34"))
        return Family::PiperSeneca;
    if (type.startsWith("PA46") || type.startsWith("P46"))
        return Family::PiperMClass;

    if (type.startsWith("DHC2")) return Family::Dhc2Beaver;
    if (type.startsWith("DHC6")) return Family::Dhc6TwinOtter;

    if (type == "BE33" || type == "BE36") return Family::BeechBonanza;
    if (type == "BE35") return Family::BeechVtailBonanza;
    if (type == "BE55" || type == "BE58") return Family::BeechBaron;
    const char* const kingAirs[] = {
        "BE9L", "BE9T", "BE10", "BE20", "BE30", "B300", "B350"};
    if (startsWithAny(type, kingAirs, sizeof(kingAirs) / sizeof(kingAirs[0])))
        return Family::BeechKingAir;
    if (type == "BE23" || type == "BE24") return Family::LightAircraft;

    if (type.startsWith("CRJ")) return Family::Crj;
    const char* const embraerEJets[] = {
        "E13", "E14", "E17", "E19", "E75", "E90", "E95"};
    if (startsWithAny(type, embraerEJets,
        sizeof(embraerEJets) / sizeof(embraerEJets[0])))
        return Family::EmbraerEJet;

    if (type.startsWith("AT4") || type.startsWith("AT7"))
        return Family::Atr;
    if (type.startsWith("DH8")) return Family::Dash8;
    if (type.startsWith("GLF") || type == "GLEX")
        return Family::Gulfstream;
    if (type.startsWith("C25") || type.startsWith("C5") ||
        type.startsWith("C6") || type.startsWith("C7"))
        return Family::Citation;
    const char* const cessnaSingles[] = {
        "C15", "C17", "C18", "C20", "C21", "C33"};
    if (startsWithAny(type, cessnaSingles,
        sizeof(cessnaSingles) / sizeof(cessnaSingles[0])))
        return Family::Cessna;

    const char* const turboprops[] = {
        "SF34", "PC12", "C208",
        "C441", "JS3", "SW4", "D328"};
    if (startsWithAny(type, turboprops, sizeof(turboprops) / sizeof(turboprops[0])) ||
        description.indexOf("TURBOPROP") >= 0)
        return Family::Turboprop;

    const char* const businessJets[] = {
        "CL3", "CL6", "FA", "LJ",
        "H25", "E35", "E50", "E55", "PC24", "BE40"};
    if (startsWithAny(type, businessJets, sizeof(businessJets) / sizeof(businessJets[0])) ||
        description.indexOf("BUSINESS") >= 0)
        return Family::BusinessJet;

    const char* const regionalJets[] = {
        "RJ",
        "BCS", "A22"};
    if (startsWithAny(type, regionalJets, sizeof(regionalJets) / sizeof(regionalJets[0])) ||
        description.indexOf("REGIONAL") >= 0)
        return Family::RegionalJet;

    if (type == "B752" || type == "B753") return Family::LongNarrowBody;

    const char* const wideBodies[] = {
        "A30", "A31", "A33", "A34", "A35", "A38", "B74", "B76", "B77",
        "B78", "DC10", "MD11", "L101"};
    if (startsWithAny(type, wideBodies, sizeof(wideBodies) / sizeof(wideBodies[0])) ||
        category == "A5")
        return Family::WideBody;

    if (category == "A1" || description.indexOf("LIGHT") >= 0 ||
        type.startsWith("C1") || type.startsWith("PA") ||
        type.startsWith("SR2") || type.startsWith("DA4"))
        return Family::LightAircraft;
    return Family::NarrowBody;
}

const lv_img_dsc_t* AircraftArtwork::imageFor(Family family)
{
    switch (family)
    {
        case Family::LongNarrowBody: return &LONG_NARROW_BODY_IMAGE;
        case Family::WideBody: return &WIDE_BODY_IMAGE;
        case Family::RegionalJet: return &REGIONAL_JET_IMAGE;
        case Family::BusinessJet: return &BUSINESS_JET_IMAGE;
        case Family::Turboprop: return &TURBOPROP_IMAGE;
        case Family::LightAircraft: return &LIGHT_AIRCRAFT_IMAGE;
        case Family::Helicopter: return &HELICOPTER_IMAGE;
        case Family::HighPerformance: return &HIGH_PERFORMANCE_IMAGE;
        case Family::Boeing747: return &BOEING_747_IMAGE;
        case Family::AirbusA380: return &AIRBUS_A380_IMAGE;
        case Family::Boeing777: return &BOEING_777_IMAGE;
        case Family::Boeing787: return &BOEING_787_IMAGE;
        case Family::AirbusA350: return &AIRBUS_A350_IMAGE;
        case Family::Crj: return &CRJ_IMAGE;
        case Family::EmbraerEJet: return &EMBRAER_EJET_IMAGE;
        case Family::Atr: return &ATR_IMAGE;
        case Family::Dash8: return &DASH8_IMAGE;
        case Family::Gulfstream: return &GULFSTREAM_IMAGE;
        case Family::Citation: return &CITATION_IMAGE;
        case Family::Cessna: return &CESSNA_IMAGE;
        case Family::PiperSuperCub: return &PIPER_SUPER_CUB_IMAGE;
        case Family::PiperCherokee: return &PIPER_CHEROKEE_IMAGE;
        case Family::PiperSeneca: return &PIPER_SENECA_IMAGE;
        case Family::PiperMClass: return &PIPER_MCLASS_IMAGE;
        case Family::Dhc2Beaver: return &DHC2_BEAVER_IMAGE;
        case Family::Dhc6TwinOtter: return &DHC6_TWIN_OTTER_IMAGE;
        case Family::BeechBonanza: return &BEECH_BONANZA_IMAGE;
        case Family::BeechVtailBonanza: return &BEECH_VTAIL_BONANZA_IMAGE;
        case Family::BeechBaron: return &BEECH_BARON_IMAGE;
        case Family::BeechKingAir: return &BEECH_KING_AIR_IMAGE;
        case Family::NarrowBody:
        default: return &NARROW_BODY_IMAGE;
    }
}

const char* AircraftArtwork::labelFor(Family family)
{
    switch (family)
    {
        case Family::LongNarrowBody: return "LONG-RANGE NARROW BODY";
        case Family::WideBody: return "WIDE-BODY AIRLINER";
        case Family::RegionalJet: return "REGIONAL JET";
        case Family::BusinessJet: return "BUSINESS JET";
        case Family::Turboprop: return "TURBOPROP";
        case Family::LightAircraft: return "LIGHT AIRCRAFT";
        case Family::Helicopter: return "HELICOPTER";
        case Family::HighPerformance: return "HIGH-PERFORMANCE";
        case Family::Boeing747: return "BOEING 747 FAMILY";
        case Family::AirbusA380: return "AIRBUS A380";
        case Family::Boeing777: return "BOEING 777 FAMILY";
        case Family::Boeing787: return "BOEING 787 FAMILY";
        case Family::AirbusA350: return "AIRBUS A350 FAMILY";
        case Family::Crj: return "CRJ FAMILY";
        case Family::EmbraerEJet: return "EMBRAER E-JET FAMILY";
        case Family::Atr: return "ATR FAMILY";
        case Family::Dash8: return "DASH 8 FAMILY";
        case Family::Gulfstream: return "GULFSTREAM FAMILY";
        case Family::Citation: return "CITATION FAMILY";
        case Family::Cessna: return "CESSNA SINGLE-ENGINE";
        case Family::PiperSuperCub: return "PIPER SUPER CUB FAMILY";
        case Family::PiperCherokee: return "PIPER CHEROKEE FAMILY";
        case Family::PiperSeneca: return "PIPER SENECA FAMILY";
        case Family::PiperMClass: return "PIPER M-CLASS FAMILY";
        case Family::Dhc2Beaver: return "DHC-2 BEAVER FAMILY";
        case Family::Dhc6TwinOtter: return "DHC-6 TWIN OTTER FAMILY";
        case Family::BeechBonanza: return "BEECH 33 / 36 BONANZA";
        case Family::BeechVtailBonanza: return "BEECH 35 V-TAIL BONANZA";
        case Family::BeechBaron: return "BEECH BARON FAMILY";
        case Family::BeechKingAir: return "BEECH KING AIR FAMILY";
        case Family::NarrowBody:
        default: return "NARROW-BODY AIRLINER";
    }
}
