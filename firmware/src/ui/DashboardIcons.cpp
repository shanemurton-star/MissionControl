#include "DashboardIcons.h"

#include "DashboardIconAssets.inc"

namespace
{
    const lv_img_dsc_t AIRCRAFT = {
        {LV_IMG_CF_RAW_ALPHA, 0, 0, 64, 64},
        sizeof(assets_icons_aircraft_png), assets_icons_aircraft_png};
    const lv_img_dsc_t LIVE_SPOTS = {
        {LV_IMG_CF_RAW_ALPHA, 0, 0, 64, 64},
        sizeof(assets_icons_live_spots_png), assets_icons_live_spots_png};
    const lv_img_dsc_t POTA = {
        {LV_IMG_CF_RAW_ALPHA, 0, 0, 64, 64},
        sizeof(assets_icons_pota_png), assets_icons_pota_png};
    const lv_img_dsc_t SATELLITE = {
        {LV_IMG_CF_RAW_ALPHA, 0, 0, 64, 64},
        sizeof(assets_icons_satellite_png), assets_icons_satellite_png};
    const lv_img_dsc_t SOLAR = {
        {LV_IMG_CF_RAW_ALPHA, 0, 0, 64, 64},
        sizeof(assets_icons_solar_png), assets_icons_solar_png};
    const lv_img_dsc_t WEATHER_PARTLY = {
        {LV_IMG_CF_RAW_ALPHA, 0, 0, 64, 64},
        sizeof(assets_icons_weather_partly_png), assets_icons_weather_partly_png};
    const lv_img_dsc_t WEATHER_SUNNY = {
        {LV_IMG_CF_RAW_ALPHA, 0, 0, 64, 64},
        sizeof(assets_icons_weather_sunny_png), assets_icons_weather_sunny_png};
    const lv_img_dsc_t WEATHER_CLOUDY = {
        {LV_IMG_CF_RAW_ALPHA, 0, 0, 64, 64},
        sizeof(assets_icons_weather_cloudy_png), assets_icons_weather_cloudy_png};
    const lv_img_dsc_t WEATHER_RAIN = {
        {LV_IMG_CF_RAW_ALPHA, 0, 0, 64, 64},
        sizeof(assets_icons_weather_rain_png), assets_icons_weather_rain_png};
    const lv_img_dsc_t WEATHER_STORM = {
        {LV_IMG_CF_RAW_ALPHA, 0, 0, 64, 64},
        sizeof(assets_icons_weather_storm_png), assets_icons_weather_storm_png};
    const lv_img_dsc_t WEATHER_SNOW = {
        {LV_IMG_CF_RAW_ALPHA, 0, 0, 64, 64},
        sizeof(assets_icons_weather_snow_png), assets_icons_weather_snow_png};
    const lv_img_dsc_t WEATHER_FOG = {
        {LV_IMG_CF_RAW_ALPHA, 0, 0, 64, 64},
        sizeof(assets_icons_weather_fog_png), assets_icons_weather_fog_png};

    const lv_img_dsc_t* weatherSource(const String& condition)
    {
        String text = condition;
        text.toUpperCase();

        if (text.indexOf("THUNDER") >= 0 || text.indexOf("STORM") >= 0)
            return &WEATHER_STORM;
        if (text.indexOf("SNOW") >= 0 || text.indexOf("SLEET") >= 0 ||
            text.indexOf("ICE") >= 0)
            return &WEATHER_SNOW;
        if (text.indexOf("RAIN") >= 0 || text.indexOf("SHOWER") >= 0 ||
            text.indexOf("DRIZZLE") >= 0)
            return &WEATHER_RAIN;
        if (text.indexOf("FOG") >= 0 || text.indexOf("MIST") >= 0 ||
            text.indexOf("HAZE") >= 0)
            return &WEATHER_FOG;
        if (text.indexOf("CLEAR") >= 0 || text.indexOf("SUNNY") >= 0)
            return &WEATHER_SUNNY;
        if (text.indexOf("OVERCAST") >= 0 ||
            (text.indexOf("CLOUD") >= 0 && text.indexOf("PART") < 0))
            return &WEATHER_CLOUDY;
        return &WEATHER_PARTLY;
    }
}

lv_obj_t* DashboardIcons::create(
    lv_obj_t* parent, Type type, int16_t x, int16_t y, uint32_t color)
{
    (void)color;
    const lv_img_dsc_t* source = &WEATHER_PARTLY;
    switch (type)
    {
        case Type::Aircraft: source = &AIRCRAFT; break;
        case Type::Radio: source = &LIVE_SPOTS; break;
        case Type::Solar: source = &SOLAR; break;
        case Type::Satellite: source = &SATELLITE; break;
        case Type::Pota: source = &POTA; break;
        case Type::Weather: break;
    }

    lv_obj_t* image = lv_img_create(parent);
    lv_img_set_src(image, source);
    lv_obj_set_pos(image, x, y);
    return image;
}

void DashboardIcons::setWeatherCondition(lv_obj_t* image, const String& condition)
{
    if (image == nullptr) return;
    lv_img_set_src(image, weatherSource(condition));
}

void DashboardIcons::warmCache(const String& weatherCondition)
{
    // Decode compressed assets before the dashboard becomes active. With the
    // old two-entry cache LVGL decoded these during the visible partial refresh,
    // which made the page appear to sweep or shift horizontally.
    const lv_color_t white = lv_color_white();
    _lv_img_cache_open(&AIRCRAFT, white, 0);
    _lv_img_cache_open(&LIVE_SPOTS, white, 0);
    _lv_img_cache_open(&POTA, white, 0);
    _lv_img_cache_open(&SATELLITE, white, 0);
    _lv_img_cache_open(&SOLAR, white, 0);
    _lv_img_cache_open(weatherSource(weatherCondition), white, 0);
    _lv_img_cache_open(&LIVE_SPOTS, lv_color_hex(0xAEB8C4), 0);
}
