# Offline world map

`world_map.png` is a 512 x 256 map generated from Natural Earth's public-domain
1:110m land dataset. It is embedded in the firmware and is not downloaded by
the panel.

Regenerate it with:

```sh
tools/generate_world_map_asset.py ne_110m_land.zip \
  assets/maps/world_map.png src/ui/WorldMapAsset.inc
```

Source: <https://www.naturalearthdata.com/downloads/110m-physical-vectors/>
Terms: <https://www.naturalearthdata.com/about/terms-of-use/>
