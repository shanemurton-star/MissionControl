#include "WeatherRadarPanel.h"

#include <WiFi.h>
#include <time.h>

#include "Theme.h"

void WeatherRadarPanel::create(
    lv_obj_t* parent,
    RadarService& radarServiceReference,
    int16_t x,
    int16_t y,
    int16_t width,
    int16_t height)
{
    radarService = &radarServiceReference;

    panel = Theme::createPanel(
        parent,
        x,
        y,
        width,
        height,
        "WEATHER RADAR");

    baseMapImage = lv_img_create(panel);
    lv_obj_add_flag(baseMapImage, LV_OBJ_FLAG_HIDDEN);

    radarImage = lv_img_create(panel);
    lv_obj_add_flag(radarImage, LV_OBJ_FLAG_HIDDEN);

    const int16_t ringSizes[3] = {70, 140, 210};
    for (uint8_t index = 0; index < 3; ++index)
    {
        rangeRings[index] = lv_obj_create(panel);
        lv_obj_set_size(rangeRings[index], ringSizes[index], ringSizes[index]);
        lv_obj_set_style_radius(rangeRings[index], LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(rangeRings[index], LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(rangeRings[index], 1, LV_PART_MAIN);
        lv_obj_set_style_border_color(rangeRings[index], Theme::color(Theme::COLOR_PRIMARY), LV_PART_MAIN);
        lv_obj_set_style_border_opa(rangeRings[index], index == 0 ? LV_OPA_50 : LV_OPA_30, LV_PART_MAIN);
        lv_obj_clear_flag(rangeRings[index], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(rangeRings[index], LV_OBJ_FLAG_HIDDEN);
    }

    locationMarker = lv_obj_create(panel);
    lv_obj_set_size(locationMarker, 9, 9);
    lv_obj_set_style_radius(locationMarker, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(locationMarker, Theme::color(Theme::COLOR_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(locationMarker, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(locationMarker, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(locationMarker, lv_color_white(), LV_PART_MAIN);
    lv_obj_clear_flag(locationMarker, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(locationMarker, LV_OBJ_FLAG_HIDDEN);

    locationLabel = Theme::createLabel(panel, "HOME", Theme::COLOR_PRIMARY);
    lv_obj_add_flag(locationLabel, LV_OBJ_FLAG_HIDDEN);

    statusLabel = Theme::createLabel(
        panel,
        "WAITING FOR RADAR",
        Theme::COLOR_WARNING);

    lv_obj_set_style_text_align(
        statusLabel,
        LV_TEXT_ALIGN_CENTER,
        LV_PART_MAIN);

    lv_obj_align(
        statusLabel,
        LV_ALIGN_CENTER,
        0,
        -4);

    attributionLabel = Theme::createLabel(
        panel,
        "IEM NEXRAD | CARTO / OSM",
        Theme::COLOR_TEXT_DIM);

    lv_obj_align(
        attributionLabel,
        LV_ALIGN_BOTTOM_RIGHT,
        0,
        0);

    update();
}

void WeatherRadarPanel::update()
{
    if (radarService == nullptr ||
        statusLabel == nullptr ||
        radarImage == nullptr ||
        baseMapImage == nullptr)
    {
        return;
    }

    if (radarService->hasBaseMap() &&
        baseMapDescriptor.data == nullptr)
    {
        showBaseMapImage();
    }

    if (radarService->isValid() &&
        radarService->getGeneration() != displayedGeneration)
    {
        showRadarImage();
    }

    updateStatusText();
}

void WeatherRadarPanel::showBaseMapImage()
{
    baseMapDescriptor.header.always_zero = 0;
    baseMapDescriptor.header.cf = LV_IMG_CF_RAW_ALPHA;
    baseMapDescriptor.header.w = 256;
    baseMapDescriptor.header.h = 256;
    baseMapDescriptor.data = radarService->getBaseMapData();
    baseMapDescriptor.data_size = radarService->getBaseMapSize();

    lv_img_set_src(baseMapImage, &baseMapDescriptor);
    lv_img_set_zoom(baseMapImage, 300);
    lv_obj_align(baseMapImage, LV_ALIGN_CENTER, 0, 5);
    lv_obj_clear_flag(baseMapImage, LV_OBJ_FLAG_HIDDEN);
    positionLocationOverlay();
}

void WeatherRadarPanel::showRadarImage()
{
    if (radarDescriptor.data != nullptr)
    {
        lv_img_cache_invalidate_src(
            &radarDescriptor);
    }

    radarDescriptor.header.always_zero = 0;
    radarDescriptor.header.cf = LV_IMG_CF_RAW_ALPHA;
    radarDescriptor.header.w = 256;
    radarDescriptor.header.h = 256;
    radarDescriptor.data =
        radarService->getImageData();
    radarDescriptor.data_size =
        radarService->getImageSize();

    lv_img_set_src(
        radarImage,
        &radarDescriptor);

    lv_img_set_zoom(
        radarImage,
        300);

    lv_obj_align(
        radarImage,
        LV_ALIGN_CENTER,
        0,
        5);

    lv_obj_clear_flag(
        radarImage,
        LV_OBJ_FLAG_HIDDEN);

    positionLocationOverlay();

    displayedGeneration =
        radarService->getGeneration();
}

void WeatherRadarPanel::positionLocationOverlay()
{
    const int16_t xOffset = static_cast<int16_t>(
        (static_cast<int32_t>(radarService->getLocationPixelX()) - 128) * 300 / 256);
    const int16_t yOffset = static_cast<int16_t>(
        (static_cast<int32_t>(radarService->getLocationPixelY()) - 128) * 300 / 256 + 5);

    for (lv_obj_t* ring : rangeRings)
    {
        lv_obj_align(ring, LV_ALIGN_CENTER, xOffset, yOffset);
        lv_obj_clear_flag(ring, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(ring);
    }

    lv_obj_align(locationMarker, LV_ALIGN_CENTER, xOffset, yOffset);
    lv_obj_clear_flag(locationMarker, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(locationMarker);

    lv_obj_align(locationLabel, LV_ALIGN_CENTER, xOffset + 25, yOffset - 11);
    lv_obj_clear_flag(locationLabel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(locationLabel);

    lv_obj_move_foreground(attributionLabel);
    lv_obj_move_foreground(statusLabel);
}

void WeatherRadarPanel::updateStatusText()
{
    if (!radarService->isValid())
    {
        lv_obj_clear_flag(
            statusLabel,
            LV_OBJ_FLAG_HIDDEN);

        if (WiFi.status() != WL_CONNECTED)
        {
            lv_label_set_text(
                statusLabel,
                "WAITING FOR WIFI");
        }
        else if (radarService->isUpdating())
        {
            lv_label_set_text(
                statusLabel,
                "DOWNLOADING RADAR...");
        }
        else if (!radarService->getLastError().isEmpty())
        {
            lv_label_set_text(
                statusLabel,
                radarService->getLastError().c_str());
        }
        else
        {
            lv_label_set_text(
                statusLabel,
                "WAITING FOR RADAR");
        }

        return;
    }

    lv_obj_add_flag(
        statusLabel,
        LV_OBJ_FLAG_HIDDEN);

    const time_t now = time(nullptr);
    const uint32_t frameTime =
        radarService->getFrameTime();

    String attribution = "IEM NEXRAD | CARTO / OSM";

    if (now > 0 &&
        frameTime > 0 &&
        static_cast<uint32_t>(now) >= frameTime)
    {
        const uint32_t ageMinutes =
            (static_cast<uint32_t>(now) - frameTime) /
            60UL;

        attribution =
            String("IEM NEXRAD  |  ") +
            String(ageMinutes) +
            " min  |  CARTO / OSM";
    }

    lv_label_set_text(
        attributionLabel,
        attribution.c_str());
}
