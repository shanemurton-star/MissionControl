# MissionControl Product Vision

## Visual contract

The MissionControl dashboard mockup is the product's visual contract. New UI
work should preserve its dark instrument-panel appearance, cyan outlines,
compact telemetry typography, green/amber status colors, and dense 800 x 480
layout.

The dashboard contains:

- A shared telemetry header with station identity, local and UTC clocks,
  connectivity, inside temperature, and power status.
- Six summary cards: Weather, Radio, Satellite Pass, Aircraft Nearby, Solar
  Conditions, and Upcoming Events.
- A bottom strip for overall status, the latest alert, and DX activity.
- Clickable summary cards that open focused detail screens. It does not use a
  persistent application navigation bar.

## Navigation contract

Screens report navigation intent with `NavigationCallback(Page)`. They never
load another screen directly. `ScreenManager` is the only component that maps
a `Page` to an LVGL screen and loads it.

Every detail screen must provide a clear route back to `Page::Dashboard`.

## Component boundaries

- `ScreenManager`: owns screens and handles page transitions.
- `DashboardScreen`: arranges summary components; it does not duplicate their
  rendering or data-formatting logic.
- `HeaderBar`: shared station, clock, network, environment, and power header.
- Dashboard panels: reusable summary components that own their widgets and
  expose click callbacks.
- Detail screens: compose shared components and detail-specific panels.
- Services: acquire, cache, and format data without owning LVGL widgets.

Planned dashboard components are `WeatherPanel`, `RadioPanel`,
`SatellitePanel`, `AircraftPanel`, `SolarPanel`, `EventsPanel`, and
`StatusBar`.

## Weather detail target

`WeatherScreen` is opened by the dashboard weather card and should contain:

- The shared `HeaderBar`.
- A dashboard/back control.
- Current conditions from `WeatherService`.
- Radar with loading, unavailable, stale-data, and refresh states.
- Hourly and multi-day forecasts.
- Active weather alerts and observation age.

Radar transport and decoding belong in a future `RadarService` and
`WeatherRadarPanel`; they should not be embedded directly in the screen.

## Delivery rules

1. Extract or add one component at a time.
2. Preserve the established layout and colors unless the visual contract is
   deliberately revised.
3. Build after each significant change.
4. Keep the last successful build working while unavailable data is represented
   by intentional placeholder states.
5. Do not duplicate service polling, value formatting, or LVGL widget ownership
   across screens.
