#include "CallsignLookupScreen.h"

#include "../ui/Theme.h"

namespace
{
    constexpr lv_coord_t PHOTO_Y = 58;
    constexpr uint16_t BANNER_PHOTO_WIDTH = 420;
    constexpr uint16_t BANNER_PHOTO_HEIGHT = 120;
    constexpr uint16_t STANDARD_PHOTO_WIDTH = 220;
    constexpr uint16_t STANDARD_PHOTO_HEIGHT = 198;

    void styleButton(lv_obj_t* button)
    {
        lv_obj_set_style_bg_color(
            button, Theme::color(Theme::COLOR_PANEL), LV_PART_MAIN);
        lv_obj_set_style_border_color(
            button, Theme::color(Theme::COLOR_PANEL_BORDER), LV_PART_MAIN);
        lv_obj_set_style_border_width(button, 1, LV_PART_MAIN);
        lv_obj_set_style_radius(button, 8, LV_PART_MAIN);
    }

    String frequencyText(uint64_t frequency)
    {
        if (frequency == 0) return "-- MHz";
        const double mhz = static_cast<double>(frequency) / 1000000.0;
        return String(mhz, 6) + " MHz";
    }
}

void CallsignLookupScreen::begin(
    ClockService& clockService,
    SettingsService& settingsServiceReference,
    WsjtxService& wsjtxService)
{
    if (screen != nullptr) return;
    settingsService = &settingsServiceReference;
    service = &wsjtxService;
    screen = lv_obj_create(nullptr);
    Theme::configureScreen(screen);
    headerBar.create(screen, clockService, Theme::SCREEN_WIDTH, Theme::HEADER_HEIGHT);
    headerBar.setSettingsCallback([this]() {
        if (navigationCallback != nullptr) navigationCallback(Page::Settings);
    });
    headerBar.setNavigationCallback([this](Page page) {
        if (navigationCallback != nullptr) navigationCallback(page);
    });

    lv_obj_t* backButton = lv_btn_create(screen);
    lv_obj_set_pos(backButton, 8, Theme::CONTENT_TOP);
    lv_obj_set_size(backButton, 116, 30);
    styleButton(backButton);
    lv_obj_add_event_cb(
        backButton, backButtonEventHandler, LV_EVENT_CLICKED, this);
    lv_obj_center(Theme::createLabel(
        backButton, LV_SYMBOL_LEFT " LIVE SPOTS", Theme::COLOR_PRIMARY,
        &lv_font_montserrat_12));

    lv_obj_t* title = Theme::createLabel(
        screen, "WSJT-X CONTACT", Theme::COLOR_PRIMARY);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 140, Theme::CONTENT_TOP + 10);

    listenerLabel = Theme::createLabel(
        screen, "", Theme::COLOR_TEXT_DIM, &lv_font_montserrat_12);
    lv_obj_set_pos(listenerLabel, 430, Theme::CONTENT_TOP + 8);
    lv_obj_set_width(listenerLabel, 360);
    lv_obj_set_style_text_align(listenerLabel, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);

    lv_obj_t* contactPanel = Theme::createPanel(
        screen, 8, 110, 328, 328, "SELECTED CONTACT");
    callLabel = Theme::createLabel(
        contactPanel, "WAITING", Theme::COLOR_TEXT, &lv_font_montserrat_28);
    lv_obj_set_pos(callLabel, 0, 44);
    lv_obj_set_width(callLabel, 300);
    lv_obj_set_style_text_align(callLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    stateLabel = Theme::createLabel(
        contactPanel, "WAITING FOR WSJT-X", Theme::COLOR_WARNING);
    lv_obj_set_pos(stateLabel, 0, 86);
    lv_obj_set_width(stateLabel, 300);
    lv_obj_set_style_text_align(stateLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    Theme::createDivider(contactPanel, 0, 120, 300);
    radioLabel = Theme::createLabel(
        contactPanel, "MODE  --\nFREQUENCY  --", Theme::COLOR_TEXT);
    lv_obj_set_pos(radioLabel, 18, 142);
    lv_obj_set_width(radioLabel, 270);

    pathLabel = Theme::createLabel(
        contactPanel, "GRID  --\nDISTANCE  --\nBEARING  --", Theme::COLOR_TEXT);
    lv_obj_set_pos(pathLabel, 18, 202);
    lv_obj_set_width(pathLabel, 270);

    lv_obj_t* callbookPanel = lv_obj_create(screen);
    lv_obj_set_pos(callbookPanel, 344, 110);
    lv_obj_set_size(callbookPanel, 448, 328);
    Theme::configurePanel(callbookPanel);
    providerLabel = Theme::createLabel(
        callbookPanel, "CALLBOOK", Theme::COLOR_PRIMARY);
    lv_obj_set_pos(providerLabel, 0, 0);
    lv_obj_set_width(providerLabel, 300);

    photoImage = lv_img_create(callbookPanel);
    lv_obj_set_pos(photoImage, 0, PHOTO_Y);
    // Keep the scaled image anchored at its top-left corner. The default
    // center pivot can move large callbook photos into the panel's clip area.
    lv_img_set_pivot(photoImage, 0, 0);
    lv_obj_add_flag(photoImage, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(photoImage, LV_OBJ_FLAG_CLICKABLE);

    photoPlaceholderLabel = Theme::createLabel(
        callbookPanel, "NO OPERATOR PHOTO", Theme::COLOR_TEXT_DIM,
        &lv_font_montserrat_12);
    lv_obj_set_pos(photoPlaceholderLabel, 0, 108);
    lv_obj_set_width(photoPlaceholderLabel, BANNER_PHOTO_WIDTH);
    lv_obj_set_style_text_align(
        photoPlaceholderLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    flagImage = lv_img_create(callbookPanel);
    lv_obj_set_pos(flagImage, 340, 0);
    lv_obj_add_flag(flagImage, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(flagImage, LV_OBJ_FLAG_CLICKABLE);

    nameLabel = Theme::createLabel(
        callbookPanel, "NO CONTACT SELECTED", Theme::COLOR_TEXT,
        &lv_font_montserrat_20);
    lv_obj_set_pos(nameLabel, 0, 184);
    lv_obj_set_size(nameLabel, 420, 30);
    lv_label_set_long_mode(nameLabel, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(nameLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    locationLabel = Theme::createLabel(
        callbookPanel, "", Theme::COLOR_TEXT_MUTED);
    lv_obj_set_pos(locationLabel, 0, 218);
    lv_obj_set_size(locationLabel, 420, 44);
    lv_label_set_long_mode(locationLabel, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(locationLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    Theme::createDivider(callbookPanel, 0, 266, 420);
    lookupStatusLabel = Theme::createLabel(
        callbookPanel, "WAITING FOR A SELECTED CONTACT",
        Theme::COLOR_TEXT_DIM, &lv_font_montserrat_12);
    lv_obj_set_pos(lookupStatusLabel, 0, 282);
    lv_obj_set_size(lookupStatusLabel, 420, 22);
    lv_label_set_long_mode(lookupStatusLabel, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(
        lookupStatusLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    update();
    updateTimer = lv_timer_create(updateTimerCallback, 500, this);
}

void CallsignLookupScreen::show()
{
    if (screen != nullptr) { lv_scr_load(screen); update(); }
}

void CallsignLookupScreen::release()
{
    if (updateTimer != nullptr) { lv_timer_del(updateTimer); updateTimer = nullptr; }
    if (screen != nullptr) { lv_obj_del(screen); screen = nullptr; }
    if (photoDescriptor.data != nullptr)
        lv_img_cache_invalidate_src(&photoDescriptor);
    if (flagDescriptor.data != nullptr)
        lv_img_cache_invalidate_src(&flagDescriptor);
    photoDescriptor = {};
    flagDescriptor = {};
    listenerLabel = callLabel = stateLabel = radioLabel = pathLabel =
        nameLabel = locationLabel = providerLabel = lookupStatusLabel = nullptr;
    photoImage = photoPlaceholderLabel = flagImage = nullptr;
    renderedRevision = renderedPacketAge = renderedPhotoGeneration =
        renderedFlagGeneration = UINT32_MAX;
}

void CallsignLookupScreen::setNavigationCallback(NavigationCallback callback)
{
    navigationCallback = callback;
}

void CallsignLookupScreen::update()
{
    if (service == nullptr || settingsService == nullptr) return;
    headerBar.update();
    const uint32_t ageSeconds = service->getLastPacketMs() == 0
        ? UINT32_MAX : (millis() - service->getLastPacketMs()) / 1000UL;
    if (renderedRevision == service->getDataRevision() &&
        renderedPacketAge == ageSeconds) return;
    renderedRevision = service->getDataRevision();
    renderedPacketAge = ageSeconds;

    const AppSettings& settings = settingsService->get();
    String listener = service->isListening() ? "LISTENING  " : "STARTING  ";
    listener += settings.wsjtxMulticastAddress + ":" + String(settings.wsjtxUdpPort);
    if (ageSeconds != UINT32_MAX)
        listener += "  |  " + String(ageSeconds) + "s AGO";
    lv_label_set_text(listenerLabel, listener.c_str());

    const String call = service->getCallsign();
    lv_label_set_text(callLabel, call.isEmpty() ? "WAITING" : call.c_str());
    lv_label_set_text(stateLabel, service->getState().c_str());

    const String radio = "MODE  " +
        (service->getMode().isEmpty() ? String("--") : service->getMode()) +
        "\nFREQUENCY  " + frequencyText(service->getDialFrequency());
    lv_label_set_text(radioLabel, radio.c_str());

    const double distance = service->getDistanceKm();
    const double bearing = service->getBearingDegrees();
    const String path = "GRID  " +
        (service->getGrid().isEmpty() ? String("--") : service->getGrid()) +
        "\nDISTANCE  " + (distance < 0.0 ? String("--") : String(distance, 0) + " km") +
        "\nBEARING  " + (bearing < 0.0 ? String("--") : String(bearing, 0) + " deg");
    lv_label_set_text(pathLabel, path.c_str());

    lv_label_set_text(providerLabel,
        (service->getProviderName() + " CALLBOOK").c_str());

    if (renderedPhotoGeneration != service->getPhotoGeneration())
    {
        renderedPhotoGeneration = service->getPhotoGeneration();
        if (photoDescriptor.data != nullptr)
            lv_img_cache_invalidate_src(&photoDescriptor);
        if (service->hasPhoto())
        {
            sidePhotoLayout =
                static_cast<uint32_t>(service->getPhotoWidth()) <=
                static_cast<uint32_t>(service->getPhotoHeight()) * 2UL;
            const uint16_t photoAreaWidth = sidePhotoLayout
                ? STANDARD_PHOTO_WIDTH : BANNER_PHOTO_WIDTH;
            const uint16_t photoAreaHeight = sidePhotoLayout
                ? STANDARD_PHOTO_HEIGHT : BANNER_PHOTO_HEIGHT;

            photoDescriptor.header.always_zero = 0;
            photoDescriptor.header.cf = LV_IMG_CF_TRUE_COLOR;
            photoDescriptor.header.w = service->getPhotoWidth();
            photoDescriptor.header.h = service->getPhotoHeight();
            photoDescriptor.data = reinterpret_cast<const uint8_t*>(
                service->getPhotoPixels());
            photoDescriptor.data_size = static_cast<uint32_t>(
                service->getPhotoWidth()) * service->getPhotoHeight() * 2UL;
            lv_img_set_src(photoImage, &photoDescriptor);
            const uint16_t zoomWidth = service->getPhotoWidth() == 0 ? 256 :
                static_cast<uint16_t>(
                    photoAreaWidth * 256UL / service->getPhotoWidth());
            const uint16_t zoomHeight = service->getPhotoHeight() == 0 ? 256 :
                static_cast<uint16_t>(
                    photoAreaHeight * 256UL / service->getPhotoHeight());
            // Permit modest enlargement of small callbook images while
            // preserving their full aspect ratio.
            const uint16_t zoom = min<uint16_t>(
                320, min(zoomWidth, zoomHeight));
            lv_img_set_zoom(photoImage, zoom);
            lv_img_set_pivot(photoImage, 0, 0);
            const lv_coord_t displayWidth = static_cast<lv_coord_t>(
                service->getPhotoWidth() * static_cast<uint32_t>(zoom) / 256UL);
            lv_obj_set_pos(
                photoImage,
                max<lv_coord_t>(0, (photoAreaWidth - displayWidth) / 2),
                PHOTO_Y);
            if (sidePhotoLayout)
            {
                lv_obj_set_pos(nameLabel, 234, 72);
                lv_obj_set_size(nameLabel, 186, 68);
                lv_obj_set_style_text_align(
                    nameLabel, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
                lv_obj_set_pos(locationLabel, 234, 148);
                lv_obj_set_size(locationLabel, 186, 106);
                lv_obj_set_style_text_align(
                    locationLabel, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
            }
            else
            {
                lv_obj_set_pos(nameLabel, 0, 184);
                lv_obj_set_size(nameLabel, 420, 30);
                lv_obj_set_style_text_align(
                    nameLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
                lv_obj_set_pos(locationLabel, 0, 218);
                lv_obj_set_size(locationLabel, 420, 44);
                lv_obj_set_style_text_align(
                    locationLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
            }
            lv_obj_clear_flag(photoImage, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(photoPlaceholderLabel, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            sidePhotoLayout = false;
            photoDescriptor = {};
            lv_obj_add_flag(photoImage, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_pos(photoPlaceholderLabel, 0, 108);
            lv_obj_set_width(photoPlaceholderLabel, BANNER_PHOTO_WIDTH);
            lv_obj_set_pos(nameLabel, 0, 184);
            lv_obj_set_size(nameLabel, 420, 30);
            lv_obj_set_style_text_align(
                nameLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
            lv_obj_set_pos(locationLabel, 0, 218);
            lv_obj_set_size(locationLabel, 420, 44);
            lv_obj_set_style_text_align(
                locationLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
            lv_obj_clear_flag(photoPlaceholderLabel, LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (renderedFlagGeneration != service->getFlagGeneration())
    {
        renderedFlagGeneration = service->getFlagGeneration();
        if (flagDescriptor.data != nullptr)
            lv_img_cache_invalidate_src(&flagDescriptor);
        if (service->hasFlag())
        {
            flagDescriptor.header.always_zero = 0;
            flagDescriptor.header.cf = LV_IMG_CF_RAW_ALPHA;
            flagDescriptor.header.w = service->getFlagWidth();
            flagDescriptor.header.h = service->getFlagHeight();
            flagDescriptor.data = service->getFlagData();
            flagDescriptor.data_size = service->getFlagSize();
            lv_img_set_src(flagImage, &flagDescriptor);
            lv_obj_set_pos(flagImage, 340, 0);
            lv_obj_clear_flag(flagImage, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            flagDescriptor = {};
            lv_obj_add_flag(flagImage, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (service->hasCallbookData())
    {
        const String name = service->getCallbookName().isEmpty()
            ? call : service->getCallbookName();
        lv_obj_set_style_text_font(
            nameLabel,
            name.length() > (sidePhotoLayout ? 20U : 36U)
                ? &lv_font_montserrat_14
                : &lv_font_montserrat_20,
            LV_PART_MAIN);
        lv_label_set_text(nameLabel, name.c_str());
        String location = service->getCallbookLocation();
        if (!service->getCallbookCountry().isEmpty())
            location += (location.isEmpty() ? "" : "  |  ") +
                service->getCallbookCountry();
        const String lookupGrid = service->getCallbookGrid();
        if (!lookupGrid.isEmpty()) location += "\nGRID  " + lookupGrid;
        lv_label_set_text(locationLabel, location.c_str());
    }
    else
    {
        lv_obj_set_style_text_font(
            nameLabel, &lv_font_montserrat_20, LV_PART_MAIN);
        lv_label_set_text(nameLabel,
            call.isEmpty() ? "NO CONTACT SELECTED" : call.c_str());
        lv_label_set_text(locationLabel,
            service->getGrid().isEmpty() ? "" : ("WSJT-X GRID  " + service->getGrid()).c_str());
    }
    lv_label_set_text(lookupStatusLabel, service->getLookupStatus().c_str());
}

void CallsignLookupScreen::backButtonEventHandler(lv_event_t* event)
{
    CallsignLookupScreen* self = static_cast<CallsignLookupScreen*>(
        lv_event_get_user_data(event));
    if (self != nullptr && self->navigationCallback != nullptr)
        self->navigationCallback(Page::LiveSpots);
}

void CallsignLookupScreen::updateTimerCallback(lv_timer_t* timer)
{
    CallsignLookupScreen* self = static_cast<CallsignLookupScreen*>(timer->user_data);
    if (self != nullptr) self->update();
}
