# MissionControl aircraft artwork

These transparent PNG assets were generated for MissionControl with OpenAI's
built-in image generation tool on 2026-08-23. They are displayed as neutral,
type-family illustrations; they do not represent a specific registration or
airline livery.

The shared generation prompt requested a polished semi-realistic technical
product illustration, crisp anti-aliased edges, subtle metallic gray/cool-blue
shading, an exact orthographic left-facing side view, transparent background,
complete aircraft framing, and no text, logos, watermarks, or scenery. Separate
subject prompts requested:

- modern narrow-body passenger jet
- elongated Boeing 757-class narrow-body jet
- modern wide-body twin jet
- compact regional jet
- rear-engine business jet
- regional turboprop
- four-seat light piston aircraft
- civilian utility helicopter
- twin-engine high-performance aircraft

The 2026-08-23 recognition expansion added distinct profiles for:

- Boeing 747, 777, and 787 families
- Airbus A350 and A380 families
- Bombardier CRJ and Embraer E-Jet families
- ATR and Dash 8 turboprops
- Gulfstream-class and Citation-class business jets
- Cessna-class high-wing single-engine aircraft
- Piper PA-18, PA-28/PA-32, PA-34, and PA-46 families
- De Havilland Canada DHC-2 and DHC-6 families
- Beechcraft 33/36 Bonanza, 35 V-tail Bonanza, Baron, and King Air families

The DHC-8 family is represented by the separate Dash 8 profile.

These are still neutral recognition illustrations rather than exact liveries.
Unknown ICAO types continue to use the nearest general family image.

The source images are resized to 280 pixels wide, then mechanically packaged as
compressed byte arrays in `src/ui/aircraft_*.inc`. `AircraftArtwork.cpp` maps
live ICAO type/category metadata to the closest family.
