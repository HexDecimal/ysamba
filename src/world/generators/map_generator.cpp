#include "./map_generator.hpp"

#include <FastNoise/FastNoise.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>
#include <cmath>

#include "./tile_rules.hpp"
#include "core/random.hpp"
#include "world/chunk.hpp"

float smoothstep(float edge0, float edge1, float x)
{
  x = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
  return x * x * (3 - 2 * x);
}

namespace dl
{
void MapGenerator::generate(const int seed, const Vector3i& offset)
{
  // TEMP
  /* m_json.load("./data/world/map_generators/terrain.json"); */
  // TEMP

  spdlog::info("=============================");
  spdlog::info("= STARTING WORLD GENERATION =");
  spdlog::info("=============================\n");
  spdlog::info("SEED: {}", seed);
  spdlog::info("WIDTH: {}", width);
  spdlog::info("HEIGHT: {}\n", height);

  const int padded_width = width + m_generation_padding * 2;
  const int padded_height = height + m_generation_padding * 2;

  chunk = std::make_unique<Chunk>(offset, true);
  chunk->tiles.set_size(width, height, depth);
  // tiles = std::vector<Cell>(width * height * depth);
  // height_map = std::vector<int>(width * height);

  auto terrain = std::vector<int>(padded_width * padded_height * depth);

  auto start = std::chrono::high_resolution_clock::now();

  spdlog::info("Generating height maps...");

  m_get_height_map(seed, offset);

  spdlog::info("Setting terrain...");

  for (int j = 0; j < padded_height; ++j)
  {
    for (int i = 0; i < padded_width; ++i)
    {
      const auto map_value = raw_height_map[j * padded_width + i];
      const int k = static_cast<int>(map_value * (depth - 1));
      bool inside_chunk = false;

      if (j >= m_generation_padding && j < padded_width - m_generation_padding && i >= m_generation_padding &&
          i < padded_height - m_generation_padding)
      {
        inside_chunk = true;
      }

      int terrain_id = 0;
      int resolved_z = k;

      if (k < 1)
      {
        terrain_id = 1;
        ++resolved_z;
      }
      else
      {
        terrain_id = 2;
      }

      if (inside_chunk)
      {
        chunk->tiles.height_map[(j - 1) * width + (i - 1)] = resolved_z;
        chunk->tiles.values[resolved_z * width * height + (j - 1) * width + (i - 1)].terrain = terrain_id;
      }

      terrain[resolved_z * padded_width * padded_height + j * padded_width + i] = terrain_id;

      for (int z = 0; z < resolved_z; ++z)
      {
        terrain[z * padded_width * padded_height + j * padded_width + i] = terrain_id;

        if (inside_chunk)
        {
          // chunk->tiles.values[z * width * height + (j - 1) * width + (i - 1)].terrain = terrain_id;
          chunk->tiles.values[z * width * height + (j - 1) * width + (i - 1)].terrain = 0;
        }
      }
    }
  }

  spdlog::info("Computing visibility...");

  chunk->tiles.compute_visibility();

  spdlog::info("Selecting tiles...");

  for (int k = 0; k < depth; ++k)
  {
    for (int j = 0; j < height; ++j)
    {
      for (int i = 0; i < width; ++i)
      {
        m_select_tile(terrain, i, j, k);
      }
    }
  }

  auto stop = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);

  spdlog::info("World generation finished! It took {} milliseconds", duration.count());
}

void MapGenerator::set_size(const Vector3i& size)
{
  width = size.x;
  height = size.y;
  depth = size.z;
}

void MapGenerator::m_get_height_map(const int seed, const Vector3i& offset)
{
  const int padded_width = width + m_generation_padding * 2;
  const int padded_height = height + m_generation_padding * 2;

  raw_height_map.resize(padded_width * padded_height);
  vegetation_type.resize(width * height);
  vegetation_density.resize(width * height);

  auto start1 = std::chrono::high_resolution_clock::now();

  /* const float frequency = 0.005f; */
  const float frequency = 0.003f;
  // (2D) (((OpenSimplex2S + FractalRidged(G0.5 W0.0, O3, L2)) + (OpenSimplex2S + SeedOffset(S10) + FractalFBm(G0.5,
  // W0.0, O5, L2.0))) + MinSmooth(S2.46))
  FastNoise::SmartNode<> elevation_noise = FastNoise::NewFromEncodedNodeTree(
      "HwAPAAMAAAAAAABAKQAAAAAAPwAAAAAAAQ0ABQAAAAAAAEAWAAoAAAApAAAAAAA/AAAAAAAApHAdQA==");
  const auto& output = elevation_noise->GenUniformGrid2D(raw_height_map.data(),
                                                         offset.x - m_generation_padding,
                                                         offset.y + m_generation_padding,
                                                         width + m_generation_padding * 2,
                                                         height + m_generation_padding * 2,
                                                         frequency,
                                                         seed);

  for (size_t i = 0; i < raw_height_map.size(); ++i)
  {
    raw_height_map[i] = smoothstep(output.min, output.max, raw_height_map[i]);
  }
  auto stop1 = std::chrono::high_resolution_clock::now();
  auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(stop1 - start1);
  auto start2 = std::chrono::high_resolution_clock::now();

  // Possible rivers
  // DwACAAAA9ijcPykAAEjhOkAAuB4FwA==
  // FastNoise::SmartNode<> river_noise =
  //     FastNoise::NewFromEncodedNodeTree("DwACAAAA9ijcPykAAEjhOkAAuB4FwA==");
  // river_noise->GenUniformGrid2D(rivers.data(), offset.x, offset.y, width, height, 0.005f, seed);

  // Vegetation type lookup
  // DAADAAAA7FG4Pw0AAwAAAAAAAEApAAAAAAA/AAAAAAAAAAAgQA==
  FastNoise::SmartNode<> vegetation_type_noise =
      FastNoise::NewFromEncodedNodeTree("DAADAAAA7FG4Pw0AAwAAAAAAAEApAAAAAAA/AAAAAAAAAAAgQA==");
  vegetation_type_noise->GenUniformGrid2D(vegetation_type.data(), offset.x, offset.y, width, height, 0.05f, seed + 30);

  auto stop2 = std::chrono::high_resolution_clock::now();
  auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(stop2 - start2);
  auto start3 = std::chrono::high_resolution_clock::now();

  // Vegetation density lookup
  // DQACAAAAexROQCkAAFK4Hj8AmpkZPw==
  FastNoise::SmartNode<> vegetation_density_noise =
      FastNoise::NewFromEncodedNodeTree("DQACAAAAexROQCkAAFK4Hj8AmpkZPw==");
  vegetation_density_noise->GenUniformGrid2D(
      vegetation_density.data(), offset.x, offset.y, width, height, 0.05f, seed + 50);

  auto stop3 = std::chrono::high_resolution_clock::now();
  auto duration3 = std::chrono::duration_cast<std::chrono::milliseconds>(stop3 - start3);

  spdlog::info(
      "INSIDE: height: {} ms, type: {} ms, density: {} ms", duration1.count(), duration2.count(), duration3.count());

  // TEMP
  // Another shape
  int index = 62;
  spdlog::debug(
      "\"bitmask\": {}, \"value\": {}", static_cast<uint32_t>(DL_EDGE_LEFT | DL_EDGE_TOP_LEFT | DL_EDGE_TOP), index++);
  spdlog::debug("\"bitmask\": {}, \"value\": {}", static_cast<uint32_t>(DL_EDGE_TOP), index++);
  spdlog::debug("\"bitmask\": {}, \"value\": {}",
                static_cast<uint32_t>(DL_EDGE_TOP | DL_EDGE_TOP_RIGHT | DL_EDGE_RIGHT),
                index++);
  spdlog::debug("\"bitmask\": {}, \"value\": {}", static_cast<uint32_t>(DL_EDGE_LEFT), index++);
  spdlog::debug("\"bitmask\": {}, \"value\": {}", static_cast<uint32_t>(DL_EDGE_NONE), index++);
  spdlog::debug("\"bitmask\": {}, \"value\": {}", static_cast<uint32_t>(DL_EDGE_RIGHT), index++);
  spdlog::debug("\"bitmask\": {}, \"value\": {}",
                static_cast<uint32_t>(DL_EDGE_LEFT | DL_EDGE_BOTTOM_LEFT | DL_EDGE_BOTTOM),
                index++);
  spdlog::debug("\"bitmask\": {}, \"value\": {}", static_cast<uint32_t>(DL_EDGE_BOTTOM), index++);
  spdlog::debug("\"bitmask\": {}, \"value\": {}",
                static_cast<uint32_t>(DL_EDGE_BOTTOM | DL_EDGE_BOTTOM_RIGHT | DL_EDGE_RIGHT),
                index++);
  // Another shape
  spdlog::debug(
      "\"bitmask\": {}, \"value\": {}",
      static_cast<uint32_t>(DL_EDGE_TOP | DL_EDGE_TOP_LEFT | DL_EDGE_LEFT | DL_EDGE_BOTTOM_LEFT | DL_EDGE_BOTTOM),
      index++);
  spdlog::debug("\"bitmask\": {}, \"value\": {}", static_cast<uint32_t>(DL_EDGE_TOP | DL_EDGE_BOTTOM), index++);
  spdlog::debug(
      "\"bitmask\": {}, \"value\": {}",
      static_cast<uint32_t>(DL_EDGE_TOP | DL_EDGE_TOP_RIGHT | DL_EDGE_RIGHT | DL_EDGE_BOTTOM_RIGHT | DL_EDGE_BOTTOM),
      index++);
  // Another shape
  spdlog::debug(
      "\"bitmask\": {}, \"value\": {}",
      static_cast<uint32_t>(DL_EDGE_LEFT | DL_EDGE_TOP_LEFT | DL_EDGE_TOP | DL_EDGE_TOP_RIGHT | DL_EDGE_RIGHT),
      index++);
  spdlog::debug("\"bitmask\": {}, \"value\": {}", static_cast<uint32_t>(DL_EDGE_LEFT | DL_EDGE_RIGHT), index++);
  spdlog::debug(
      "\"bitmask\": {}, \"value\": {}",
      static_cast<uint32_t>(DL_EDGE_LEFT | DL_EDGE_BOTTOM_LEFT | DL_EDGE_BOTTOM | DL_EDGE_BOTTOM_RIGHT | DL_EDGE_RIGHT),
      index++);
  // Another shape
  spdlog::debug("\"bitmask\": {}, \"value\": {}",
                static_cast<uint32_t>(DL_EDGE_TOP_LEFT | DL_EDGE_TOP | DL_EDGE_TOP_RIGHT | DL_EDGE_RIGHT |
                                      DL_EDGE_BOTTOM_RIGHT | DL_EDGE_BOTTOM | DL_EDGE_BOTTOM_LEFT | DL_EDGE_LEFT),
                index++);
  // Another shape
  spdlog::debug("\"bitmask\": {}, \"value\": {}",
                static_cast<uint32_t>(DL_EDGE_TOP_LEFT | DL_EDGE_TOP | DL_EDGE_TOP_RIGHT | DL_EDGE_BOTTOM_RIGHT |
                                      DL_EDGE_BOTTOM_LEFT | DL_EDGE_LEFT),
                index++);
  spdlog::debug("\"bitmask\": {}, \"value\": {}",
                static_cast<uint32_t>(DL_EDGE_TOP_LEFT | DL_EDGE_TOP | DL_EDGE_TOP_RIGHT | DL_EDGE_BOTTOM_RIGHT),
                index++);
  spdlog::debug("\"bitmask\": {}, \"value\": {}",
                static_cast<uint32_t>(DL_EDGE_TOP_LEFT | DL_EDGE_TOP | DL_EDGE_TOP_RIGHT | DL_EDGE_BOTTOM_LEFT),
                index++);
  spdlog::debug("\"bitmask\": {}, \"value\": {}",
                static_cast<uint32_t>(DL_EDGE_TOP_LEFT | DL_EDGE_TOP | DL_EDGE_TOP_RIGHT | DL_EDGE_BOTTOM_LEFT |
                                      DL_EDGE_RIGHT | DL_EDGE_BOTTOM_RIGHT),
                index++);
  spdlog::debug("\"bitmask\": {}, \"value\": {}",
                static_cast<uint32_t>(DL_EDGE_TOP_LEFT | DL_EDGE_LEFT | DL_EDGE_BOTTOM_LEFT | DL_EDGE_BOTTOM_RIGHT),
                index++);
  spdlog::debug("\"bitmask\": {}, \"value\": {}", static_cast<uint32_t>(DL_EDGE_BOTTOM_RIGHT), index++);
  spdlog::debug("\"bitmask\": {}, \"value\": {}", static_cast<uint32_t>(DL_EDGE_BOTTOM_LEFT), index++);
  spdlog::debug("\"bitmask\": {}, \"value\": {}",
                static_cast<uint32_t>(DL_EDGE_TOP_RIGHT | DL_EDGE_RIGHT | DL_EDGE_TOP_RIGHT | DL_EDGE_BOTTOM_LEFT),
                index++);
  spdlog::debug("\"bitmask\": {}, \"value\": {}",
                static_cast<uint32_t>(DL_EDGE_TOP_LEFT | DL_EDGE_LEFT | DL_EDGE_BOTTOM_LEFT | DL_EDGE_TOP_RIGHT),
                index++);
  spdlog::debug("\"bitmask\": {}, \"value\": {}", static_cast<uint32_t>(DL_EDGE_TOP_RIGHT), index++);
  spdlog::debug("\"bitmask\": {}, \"value\": {}", static_cast<uint32_t>(DL_EDGE_TOP_LEFT), index++);
  spdlog::debug("\"bitmask\": {}, \"value\": {}",
                static_cast<uint32_t>(DL_EDGE_TOP_RIGHT | DL_EDGE_RIGHT | DL_EDGE_BOTTOM_RIGHT | DL_EDGE_TOP_LEFT),
                index++);
  spdlog::debug("\"bitmask\": {}, \"value\": {}",
                static_cast<uint32_t>(DL_EDGE_BOTTOM_LEFT | DL_EDGE_BOTTOM | DL_EDGE_BOTTOM_RIGHT | DL_EDGE_TOP_RIGHT |
                                      DL_EDGE_TOP_LEFT | DL_EDGE_LEFT),
                index++);
  spdlog::debug("\"bitmask\": {}, \"value\": {}",
                static_cast<uint32_t>(DL_EDGE_BOTTOM_LEFT | DL_EDGE_BOTTOM | DL_EDGE_BOTTOM_RIGHT | DL_EDGE_TOP_RIGHT),
                index++);
  spdlog::debug("\"bitmask\": {}, \"value\": {}",
                static_cast<uint32_t>(DL_EDGE_BOTTOM_LEFT | DL_EDGE_BOTTOM | DL_EDGE_BOTTOM_RIGHT | DL_EDGE_TOP_LEFT),
                index++);
  spdlog::debug("\"bitmask\": {}, \"value\": {}",
                static_cast<uint32_t>(DL_EDGE_BOTTOM_LEFT | DL_EDGE_BOTTOM | DL_EDGE_BOTTOM_RIGHT | DL_EDGE_TOP_RIGHT |
                                      DL_EDGE_TOP_LEFT | DL_EDGE_RIGHT),
                index++);
  // Another shape
  spdlog::debug("\"bitmask\": {}, \"value\": {}",
                static_cast<uint32_t>(DL_EDGE_TOP_LEFT | DL_EDGE_LEFT | DL_EDGE_BOTTOM_LEFT | DL_EDGE_TOP_RIGHT |
                                      DL_EDGE_BOTTOM_RIGHT),
                index++);
  spdlog::debug(
      "\"bitmask\": {}, \"value\": {}", static_cast<uint32_t>(DL_EDGE_TOP_RIGHT | DL_EDGE_BOTTOM_RIGHT), index++);
  spdlog::debug(
      "\"bitmask\": {}, \"value\": {}", static_cast<uint32_t>(DL_EDGE_TOP_LEFT | DL_EDGE_BOTTOM_LEFT), index++);
  spdlog::debug("\"bitmask\": {}, \"value\": {}",
                static_cast<uint32_t>(DL_EDGE_TOP_RIGHT | DL_EDGE_RIGHT | DL_EDGE_BOTTOM_RIGHT | DL_EDGE_TOP_LEFT |
                                      DL_EDGE_BOTTOM_LEFT),
                index++);
  // Another shape
  spdlog::debug("\"bitmask\": {}, \"value\": {}",
                static_cast<uint32_t>(DL_EDGE_TOP_LEFT | DL_EDGE_TOP | DL_EDGE_TOP_RIGHT | DL_EDGE_BOTTOM_LEFT |
                                      DL_EDGE_BOTTOM_RIGHT),
                index++);
  spdlog::debug(
      "\"bitmask\": {}, \"value\": {}", static_cast<uint32_t>(DL_EDGE_BOTTOM_LEFT | DL_EDGE_BOTTOM_RIGHT), index++);
  spdlog::debug("\"bitmask\": {}, \"value\": {}", static_cast<uint32_t>(DL_EDGE_TOP_LEFT | DL_EDGE_TOP_RIGHT), index++);
  spdlog::debug("\"bitmask\": {}, \"value\": {}",
                static_cast<uint32_t>(DL_EDGE_BOTTOM_LEFT | DL_EDGE_BOTTOM | DL_EDGE_BOTTOM_RIGHT | DL_EDGE_TOP_LEFT |
                                      DL_EDGE_TOP_RIGHT),
                index++);
  // Another shape
  spdlog::debug(
      "\"bitmask\": {}, \"value\": {}",
      static_cast<uint32_t>(DL_EDGE_TOP_LEFT | DL_EDGE_TOP_RIGHT | DL_EDGE_BOTTOM_LEFT | DL_EDGE_BOTTOM_RIGHT),
      index++);
  // Another shape
  spdlog::debug(
      "\"bitmask\": {}, \"value\": {}", static_cast<uint32_t>(DL_EDGE_TOP_RIGHT | DL_EDGE_BOTTOM_LEFT), index++);
  spdlog::debug(
      "\"bitmask\": {}, \"value\": {}", static_cast<uint32_t>(DL_EDGE_TOP_LEFT | DL_EDGE_BOTTOM_RIGHT), index++);
  spdlog::debug("\"bitmask\": {}, \"value\": {}",
                static_cast<uint32_t>(DL_EDGE_TOP_LEFT | DL_EDGE_TOP_RIGHT | DL_EDGE_BOTTOM_LEFT),
                index++);
  spdlog::debug("\"bitmask\": {}, \"value\": {}",
                static_cast<uint32_t>(DL_EDGE_TOP_LEFT | DL_EDGE_BOTTOM_RIGHT | DL_EDGE_BOTTOM_LEFT),
                index++);
  spdlog::debug("\"bitmask\": {}, \"value\": {}",
                static_cast<uint32_t>(DL_EDGE_TOP_LEFT | DL_EDGE_TOP_RIGHT | DL_EDGE_BOTTOM_RIGHT),
                index++);
  spdlog::debug("\"bitmask\": {}, \"value\": {}",
                static_cast<uint32_t>(DL_EDGE_TOP_RIGHT | DL_EDGE_BOTTOM_RIGHT | DL_EDGE_BOTTOM_LEFT),
                index++);

  /* const auto simplex_freq = m_json.object["simplex_freq"].get<float>(); */
  /* const auto simplex_octaves = m_json.object["simplex_octaves"].get<int>(); */
  /* const auto simplex_lacunarity = m_json.object["simplex_lacunarity"].get<float>(); */
  /* const auto simplex_gain = m_json.object["simplex_gain"].get<float>(); */
  /* const auto simplex_weighted_strength = m_json.object["simplex_weighted_strength"].get<float>(); */

  /* auto noise = FastNoiseLite{seed}; */
  /* noise.SetSeed(seed); */
  /* noise.SetNoiseType(FastNoiseLite::NoiseType::NoiseType_OpenSimplex2S); */
  /* noise.SetRotationType3D(FastNoiseLite::RotationType3D_ImproveXYPlanes); */
  /* noise.SetFrequency(simplex_freq); */
  /* noise.SetFractalType(FastNoiseLite::FractalType::FractalType_FBm); */
  /* noise.SetFractalOctaves(simplex_octaves); */
  /* noise.SetFractalLacunarity(simplex_lacunarity); */
  /* noise.SetFractalGain(simplex_gain); */
  /* noise.SetFractalWeightedStrength(simplex_weighted_strength); */

  /* float max_value = 0.0; */
  /* float min_value = 1.0; */

  /* for (int j = 0; j < height; ++j) */
  /* { */
  /*   for (int i = 0; i < width; ++i) */
  /*   { */
  /*     /1* const float gradient = m_get_rectangle_gradient_value(i, j); *1/ */
  /*     /1* const float noise_value = noise.GetNoise(static_cast<float>(offset.x + i), static_cast<float>(offset.y +
   * j)); *1/ */

  /*     /1* height_values[j * width + i] = noise_value; *1/ */
  /*     max_value = std::max(max_value, height_values[j*width + i]); */
  /*     min_value = std::min(min_value, height_values[j*width + i]); */
  /*   } */
  /* } */

  /* spdlog::debug("MIN MAX {} {}", min_value, max_value); */

  /* const double distance = max_value - min_value; */
  /* const double distance = 0.9; */
}

float MapGenerator::m_get_rectangle_gradient_value(const int x, const int y)
{
  auto distance_to_edge = std::min(abs(x - width), x);
  distance_to_edge = std::min(distance_to_edge, abs(y - height));
  distance_to_edge = std::min(distance_to_edge, y) * 2;
  return 1.f - static_cast<float>(distance_to_edge) / (width / 2.0f);
}

void MapGenerator::m_select_tile(const std::vector<int>& terrain, const int x, const int y, const int z)
{
  const int padded_width = width + m_generation_padding * 2;
  const int padded_height = height + m_generation_padding * 2;
  const int transposed_x = x + m_generation_padding;
  const int transposed_y = y + m_generation_padding;

  const auto terrain_id = terrain[z * padded_width * padded_height + transposed_y * padded_width + transposed_x];

  if (terrain_id == 0)
  {
    return;
  }

  // TODO: Set non walkable block tile if not visible
  if (!chunk->tiles.has_flags(DL_CELL_FLAG_VISIBLE, x, y, z))
  {
    chunk->tiles.values[z * width * height + y * width + x].terrain = 1;
    return;
  }

  TileValues old_values{terrain_id, 0};
  TileValues new_values{terrain_id, 0};

  do
  {
    old_values.terrain = new_values.terrain;
    old_values.decoration = new_values.decoration;

    const auto& rule_object = TileRules::get(old_values.terrain);
    const auto index = rule_object.index();

    switch (index)
    {
    case 0:
      break;

    case 1:
    {
      const auto& rule = std::get<AutoTile4SidesRule>(rule_object);
      const auto bitmask = m_get_bitmask_4_sided(terrain, transposed_x, transposed_y, z, rule.neighbor);
      new_values.terrain = rule.output[bitmask].value;
      break;
    }

    case 3:
    {
      const auto& rule = std::get<UniformDistributionRule>(rule_object);
      const auto prob = random::get_real();
      double cumulative_probability = 0.0;

      for (const auto& transform : rule.output)
      {
        cumulative_probability += transform.probability;

        if (prob < cumulative_probability)
        {
          if (transform.placement == PlacementType::Terrain)
          {
            new_values.terrain = transform.value;
          }
          else
          {
            new_values.decoration = transform.value;
          }
          break;
        }
      }
      break;
    }

    case 2:
    {
      const auto& rule = std::get<AutoTile8SidesRule>(rule_object);
      const auto bitmask = m_get_bitmask_8_sided(terrain, transposed_x, transposed_y, z, rule.neighbor, rule.input);
      int new_terrain_id = 0;

      switch (bitmask)
      {
      case DL_EDGE_RIGHT | DL_EDGE_BOTTOM_RIGHT | DL_EDGE_BOTTOM:
        new_terrain_id = rule.output[0].value;
        break;
      case DL_EDGE_LEFT | DL_EDGE_BOTTOM_LEFT | DL_EDGE_BOTTOM | DL_EDGE_BOTTOM_RIGHT | DL_EDGE_RIGHT:
        new_terrain_id = rule.output[1].value;
        break;
      case DL_EDGE_LEFT | DL_EDGE_BOTTOM_LEFT | DL_EDGE_BOTTOM:
        new_terrain_id = rule.output[2].value;
        break;
      case DL_EDGE_TOP | DL_EDGE_TOP_RIGHT | DL_EDGE_RIGHT | DL_EDGE_BOTTOM_RIGHT | DL_EDGE_BOTTOM:
        new_terrain_id = rule.output[3].value;
        break;
      case DL_EDGE_TOP_LEFT | DL_EDGE_TOP | DL_EDGE_TOP_RIGHT | DL_EDGE_RIGHT | DL_EDGE_BOTTOM_RIGHT | DL_EDGE_BOTTOM |
          DL_EDGE_BOTTOM_LEFT | DL_EDGE_LEFT:
        new_terrain_id = rule.output[4].value;
        break;
      case DL_EDGE_TOP | DL_EDGE_TOP_LEFT | DL_EDGE_LEFT | DL_EDGE_BOTTOM_LEFT | DL_EDGE_BOTTOM:
        new_terrain_id = rule.output[5].value;
        break;
      case DL_EDGE_TOP | DL_EDGE_TOP_RIGHT | DL_EDGE_RIGHT:
        new_terrain_id = rule.output[6].value;
        break;
      case DL_EDGE_LEFT | DL_EDGE_TOP_LEFT | DL_EDGE_TOP | DL_EDGE_TOP_RIGHT | DL_EDGE_RIGHT:
        new_terrain_id = rule.output[7].value;
        break;
      case DL_EDGE_LEFT | DL_EDGE_TOP_LEFT | DL_EDGE_TOP:
        new_terrain_id = rule.output[8].value;
        break;
      case DL_EDGE_RIGHT:
        new_terrain_id = rule.output[9].value;
        break;
      case DL_EDGE_LEFT | DL_EDGE_RIGHT:
        new_terrain_id = rule.output[10].value;
        break;
      case DL_EDGE_LEFT:
        new_terrain_id = rule.output[11].value;
        break;
      case DL_EDGE_BOTTOM:
        new_terrain_id = rule.output[12].value;
        break;
      case DL_EDGE_TOP | DL_EDGE_BOTTOM:
        new_terrain_id = rule.output[13].value;
        break;
      case DL_EDGE_TOP:
        new_terrain_id = rule.output[14].value;
        break;
      case DL_EDGE_NONE:
        new_terrain_id = rule.output[15].value;
        break;
      case DL_EDGE_BOTTOM | DL_EDGE_RIGHT:
        new_terrain_id = rule.output[16].value;
        break;
      case DL_EDGE_LEFT | DL_EDGE_BOTTOM_LEFT | DL_EDGE_BOTTOM | DL_EDGE_RIGHT:
        new_terrain_id = rule.output[17].value;
        break;
      case DL_EDGE_LEFT | DL_EDGE_BOTTOM | DL_EDGE_BOTTOM_RIGHT | DL_EDGE_RIGHT:
        new_terrain_id = rule.output[18].value;
        break;
      case DL_EDGE_LEFT | DL_EDGE_BOTTOM:
        new_terrain_id = rule.output[19].value;
        break;
      case DL_EDGE_BOTTOM | DL_EDGE_RIGHT | DL_EDGE_TOP_RIGHT | DL_EDGE_TOP:
        new_terrain_id = rule.output[20].value;
        break;
      case DL_EDGE_LEFT | DL_EDGE_BOTTOM_LEFT | DL_EDGE_BOTTOM | DL_EDGE_RIGHT | DL_EDGE_TOP_RIGHT | DL_EDGE_TOP |
          DL_EDGE_TOP_LEFT:
        new_terrain_id = rule.output[21].value;
        break;
      case DL_EDGE_LEFT | DL_EDGE_BOTTOM | DL_EDGE_BOTTOM_RIGHT | DL_EDGE_RIGHT | DL_EDGE_TOP_RIGHT | DL_EDGE_TOP |
          DL_EDGE_TOP_LEFT:
        new_terrain_id = rule.output[22].value;
        break;
      case DL_EDGE_TOP | DL_EDGE_TOP_LEFT | DL_EDGE_LEFT | DL_EDGE_BOTTOM:
        new_terrain_id = rule.output[23].value;
        break;
      case DL_EDGE_TOP | DL_EDGE_RIGHT | DL_EDGE_BOTTOM_RIGHT | DL_EDGE_BOTTOM:
        new_terrain_id = rule.output[24].value;
        break;
      case DL_EDGE_TOP | DL_EDGE_RIGHT | DL_EDGE_BOTTOM_RIGHT | DL_EDGE_BOTTOM | DL_EDGE_BOTTOM_LEFT | DL_EDGE_LEFT |
          DL_EDGE_TOP_LEFT:
        new_terrain_id = rule.output[25].value;
        break;
      case DL_EDGE_TOP | DL_EDGE_TOP_RIGHT | DL_EDGE_RIGHT | DL_EDGE_BOTTOM_RIGHT | DL_EDGE_BOTTOM |
          DL_EDGE_BOTTOM_LEFT | DL_EDGE_LEFT:
        new_terrain_id = rule.output[26].value;
        break;
      case DL_EDGE_TOP | DL_EDGE_BOTTOM | DL_EDGE_BOTTOM_LEFT | DL_EDGE_LEFT:
        new_terrain_id = rule.output[27].value;
        break;
      case DL_EDGE_TOP | DL_EDGE_RIGHT:
        new_terrain_id = rule.output[28].value;
        break;
      case DL_EDGE_TOP | DL_EDGE_RIGHT | DL_EDGE_LEFT | DL_EDGE_TOP_LEFT | DL_EDGE_TOP:
        new_terrain_id = rule.output[29].value;
        break;
      case DL_EDGE_TOP | DL_EDGE_TOP_RIGHT | DL_EDGE_RIGHT | DL_EDGE_LEFT:
        new_terrain_id = rule.output[30].value;
        break;
      case DL_EDGE_TOP | DL_EDGE_LEFT:
        new_terrain_id = rule.output[31].value;
        break;
      case DL_EDGE_TOP | DL_EDGE_RIGHT | DL_EDGE_BOTTOM:
        new_terrain_id = rule.output[32].value;
        break;
      case DL_EDGE_TOP | DL_EDGE_RIGHT | DL_EDGE_BOTTOM | DL_EDGE_BOTTOM_LEFT | DL_EDGE_LEFT | DL_EDGE_TOP_LEFT:
        new_terrain_id = rule.output[33].value;
        break;
      case DL_EDGE_TOP | DL_EDGE_TOP_RIGHT | DL_EDGE_RIGHT | DL_EDGE_BOTTOM_RIGHT | DL_EDGE_BOTTOM | DL_EDGE_LEFT:
        new_terrain_id = rule.output[34].value;
        break;
      case DL_EDGE_TOP | DL_EDGE_LEFT | DL_EDGE_BOTTOM:
        new_terrain_id = rule.output[35].value;
        break;
      case DL_EDGE_LEFT | DL_EDGE_BOTTOM | DL_EDGE_RIGHT:
        new_terrain_id = rule.output[36].value;
        break;
      case DL_EDGE_LEFT | DL_EDGE_TOP_LEFT | DL_EDGE_TOP | DL_EDGE_TOP_RIGHT | DL_EDGE_RIGHT | DL_EDGE_BOTTOM:
        new_terrain_id = rule.output[37].value;
        break;
      case DL_EDGE_TOP | DL_EDGE_RIGHT | DL_EDGE_BOTTOM_RIGHT | DL_EDGE_BOTTOM | DL_EDGE_BOTTOM_LEFT | DL_EDGE_LEFT:
        new_terrain_id = rule.output[38].value;
        break;
      case DL_EDGE_LEFT | DL_EDGE_TOP | DL_EDGE_RIGHT:
        new_terrain_id = rule.output[39].value;
        break;
      case DL_EDGE_TOP | DL_EDGE_RIGHT | DL_EDGE_BOTTOM | DL_EDGE_LEFT:
        new_terrain_id = rule.output[40].value;
        break;
      case DL_EDGE_LEFT | DL_EDGE_TOP_LEFT | DL_EDGE_TOP | DL_EDGE_RIGHT | DL_EDGE_BOTTOM_RIGHT | DL_EDGE_BOTTOM:
        new_terrain_id = rule.output[41].value;
        break;
      case DL_EDGE_TOP | DL_EDGE_TOP_RIGHT | DL_EDGE_RIGHT | DL_EDGE_BOTTOM | DL_EDGE_BOTTOM_LEFT | DL_EDGE_LEFT:
        new_terrain_id = rule.output[42].value;
        break;
      case DL_EDGE_TOP | DL_EDGE_RIGHT | DL_EDGE_BOTTOM_RIGHT | DL_EDGE_BOTTOM | DL_EDGE_LEFT:
        new_terrain_id = rule.output[43].value;
        break;
      case DL_EDGE_TOP | DL_EDGE_TOP_RIGHT | DL_EDGE_RIGHT | DL_EDGE_BOTTOM | DL_EDGE_LEFT:
        new_terrain_id = rule.output[44].value;
        break;
      case DL_EDGE_TOP | DL_EDGE_RIGHT | DL_EDGE_BOTTOM | DL_EDGE_BOTTOM_LEFT | DL_EDGE_LEFT:
        new_terrain_id = rule.output[45].value;
        break;
      case DL_EDGE_TOP | DL_EDGE_RIGHT | DL_EDGE_BOTTOM | DL_EDGE_LEFT | DL_EDGE_TOP_LEFT:
        new_terrain_id = rule.output[46].value;
        break;
      }

      // switch (bitmask)
      // {
      //   // Another shape
      //   case DL_EDGE_LEFT | DL_EDGE_TOP_LEFT | DL_EDGE_TOP:
      //     new_terrain_id = rule.output[0].value;
      //     break;
      //   case DL_EDGE_TOP:
      //     new_terrain_id = rule.output[1].value;
      //     break;
      //   case DL_EDGE_TOP | DL_EDGE_TOP_RIGHT | DL_EDGE_RIGHT:
      //     new_terrain_id = rule.output[2].value;
      //     break;
      //   case DL_EDGE_LEFT:
      //     new_terrain_id = rule.output[3].value;
      //     break;
      //   case DL_EDGE_NONE:
      //     new_terrain_id = rule.output[4].value;
      //     break;
      //   case DL_EDGE_RIGHT:
      //     new_terrain_id = rule.output[5].value;
      //     break;
      //   case DL_EDGE_LEFT | DL_EDGE_BOTTOM_LEFT | DL_EDGE_BOTTOM:
      //     new_terrain_id = rule.output[6].value;
      //     break;
      //   case DL_EDGE_BOTTOM:
      //     new_terrain_id = rule.output[7].value;
      //     break;
      //   case DL_EDGE_BOTTOM | DL_EDGE_BOTTOM_RIGHT | DL_EDGE_RIGHT:
      //     new_terrain_id = rule.output[8].value;
      //     break;
      //   // Another shape
      //   case DL_EDGE_TOP | DL_EDGE_TOP_LEFT | DL_EDGE_LEFT | DL_EDGE_BOTTOM_LEFT | DL_EDGE_BOTTOM:
      //     new_terrain_id = rule.output[9].value;
      //     break;
      //   case DL_EDGE_TOP | DL_EDGE_BOTTOM:
      //     new_terrain_id = rule.output[10].value;
      //     break;
      //   case DL_EDGE_TOP | DL_EDGE_TOP_RIGHT | DL_EDGE_RIGHT | DL_EDGE_BOTTOM_RIGHT | DL_EDGE_BOTTOM:
      //     new_terrain_id = rule.output[11].value;
      //     break;
      //   // Another shape
      //   case DL_EDGE_LEFT | DL_EDGE_TOP_LEFT | DL_EDGE_TOP | DL_EDGE_TOP_RIGHT | DL_EDGE_RIGHT:
      //     new_terrain_id = rule.output[12].value;
      //     break;
      //   case DL_EDGE_LEFT | DL_EDGE_RIGHT:
      //     new_terrain_id = rule.output[13].value;
      //     break;
      //   case DL_EDGE_LEFT | DL_EDGE_BOTTOM_LEFT | DL_EDGE_BOTTOM | DL_EDGE_BOTTOM_RIGHT | DL_EDGE_RIGHT:
      //     new_terrain_id = rule.output[14].value;
      //     break;
      //   // Another shape
      //   case DL_EDGE_TOP_LEFT | DL_EDGE_TOP | DL_EDGE_TOP_RIGHT | DL_EDGE_RIGHT | DL_EDGE_BOTTOM_RIGHT |
      //   DL_EDGE_BOTTOM | DL_EDGE_BOTTOM_LEFT | DL_EDGE_LEFT:
      //     new_terrain_id = rule.output[15].value;
      //     break;
      //   // Another shape
      //   case DL_EDGE_TOP_LEFT | DL_EDGE_TOP | DL_EDGE_TOP_RIGHT | DL_EDGE_BOTTOM_RIGHT | DL_EDGE_BOTTOM_LEFT |
      //   DL_EDGE_LEFT:
      //     new_terrain_id = rule.output[16].value;
      //     break;
      //   case DL_EDGE_TOP_LEFT | DL_EDGE_TOP | DL_EDGE_TOP_RIGHT | DL_EDGE_BOTTOM_RIGHT:
      //     new_terrain_id = rule.output[17].value;
      //     break;
      //   case DL_EDGE_TOP_LEFT | DL_EDGE_TOP | DL_EDGE_TOP_RIGHT | DL_EDGE_BOTTOM_LEFT:
      //     new_terrain_id = rule.output[18].value;
      //     break;
      //   case DL_EDGE_TOP_LEFT | DL_EDGE_TOP | DL_EDGE_TOP_RIGHT | DL_EDGE_BOTTOM_LEFT | DL_EDGE_RIGHT |
      //   DL_EDGE_BOTTOM_RIGHT:
      //     new_terrain_id = rule.output[19].value;
      //     break;
      //   case DL_EDGE_TOP_LEFT | DL_EDGE_LEFT | DL_EDGE_BOTTOM_LEFT | DL_EDGE_BOTTOM_RIGHT:
      //     new_terrain_id = rule.output[20].value;
      //     break;
      //   case DL_EDGE_BOTTOM_RIGHT:
      //     new_terrain_id = rule.output[21].value;
      //     break;
      //   case DL_EDGE_BOTTOM_LEFT:
      //     new_terrain_id = rule.output[22].value;
      //     break;
      //   case DL_EDGE_TOP_RIGHT | DL_EDGE_RIGHT | DL_EDGE_TOP_RIGHT | DL_EDGE_BOTTOM_LEFT:
      //     new_terrain_id = rule.output[23].value;
      //     break;
      //   case DL_EDGE_TOP_LEFT | DL_EDGE_LEFT | DL_EDGE_BOTTOM_LEFT | DL_EDGE_TOP_RIGHT:
      //     new_terrain_id = rule.output[24].value;
      //     break;
      //   case DL_EDGE_TOP_RIGHT:
      //     new_terrain_id = rule.output[25].value;
      //     break;
      //   case DL_EDGE_TOP_LEFT:
      //     new_terrain_id = rule.output[26].value;
      //     break;
      //   case DL_EDGE_TOP_RIGHT | DL_EDGE_RIGHT | DL_EDGE_BOTTOM_RIGHT | DL_EDGE_TOP_LEFT:
      //     new_terrain_id = rule.output[27].value;
      //     break;
      //   case DL_EDGE_BOTTOM_LEFT | DL_EDGE_BOTTOM | DL_EDGE_BOTTOM_RIGHT | DL_EDGE_TOP_RIGHT | DL_EDGE_TOP_LEFT |
      //   DL_EDGE_LEFT:
      //     new_terrain_id = rule.output[28].value;
      //     break;
      //   case DL_EDGE_BOTTOM_LEFT | DL_EDGE_BOTTOM | DL_EDGE_BOTTOM_RIGHT | DL_EDGE_TOP_RIGHT:
      //     new_terrain_id = rule.output[29].value;
      //     break;
      //   case DL_EDGE_BOTTOM_LEFT | DL_EDGE_BOTTOM | DL_EDGE_BOTTOM_RIGHT | DL_EDGE_TOP_LEFT:
      //     new_terrain_id = rule.output[30].value;
      //     break;
      //   case DL_EDGE_BOTTOM_LEFT | DL_EDGE_BOTTOM | DL_EDGE_BOTTOM_RIGHT | DL_EDGE_TOP_RIGHT | DL_EDGE_TOP_LEFT |
      //   DL_EDGE_RIGHT:
      //     new_terrain_id = rule.output[31].value;
      //     break;
      //   // Another shape
      //   case DL_EDGE_TOP_LEFT | DL_EDGE_LEFT | DL_EDGE_BOTTOM_LEFT | DL_EDGE_TOP_RIGHT | DL_EDGE_BOTTOM_RIGHT:
      //     new_terrain_id = rule.output[32].value;
      //     break;
      //   case DL_EDGE_TOP_RIGHT | DL_EDGE_BOTTOM_RIGHT:
      //     new_terrain_id = rule.output[33].value;
      //     break;
      //   case DL_EDGE_TOP_LEFT | DL_EDGE_BOTTOM_LEFT:
      //     new_terrain_id = rule.output[34].value;
      //     break;
      //   case DL_EDGE_TOP_RIGHT | DL_EDGE_RIGHT | DL_EDGE_BOTTOM_RIGHT | DL_EDGE_TOP_LEFT | DL_EDGE_BOTTOM_LEFT:
      //     new_terrain_id = rule.output[35].value;
      //     break;
      //   // Another shape
      //   case DL_EDGE_TOP_LEFT | DL_EDGE_TOP | DL_EDGE_TOP_RIGHT | DL_EDGE_BOTTOM_LEFT | DL_EDGE_BOTTOM_RIGHT:
      //     new_terrain_id = rule.output[36].value;
      //     break;
      //   case DL_EDGE_BOTTOM_LEFT | DL_EDGE_BOTTOM_RIGHT:
      //     new_terrain_id = rule.output[37].value;
      //     break;
      //   case DL_EDGE_TOP_LEFT | DL_EDGE_TOP_RIGHT:
      //     new_terrain_id = rule.output[38].value;
      //     break;
      //   case DL_EDGE_BOTTOM_LEFT | DL_EDGE_BOTTOM | DL_EDGE_BOTTOM_RIGHT | DL_EDGE_TOP_LEFT | DL_EDGE_TOP_RIGHT:
      //     new_terrain_id = rule.output[39].value;
      //     break;
      //   // Another shape
      //   case DL_EDGE_TOP_LEFT | DL_EDGE_TOP_RIGHT | DL_EDGE_BOTTOM_LEFT | DL_EDGE_BOTTOM_RIGHT:
      //     new_terrain_id = rule.output[40].value;
      //     break;
      //   // Another shape
      //   case DL_EDGE_TOP_RIGHT | DL_EDGE_BOTTOM_LEFT:
      //     new_terrain_id = rule.output[41].value;
      //     break;
      //   case DL_EDGE_TOP_LEFT | DL_EDGE_BOTTOM_RIGHT:
      //     new_terrain_id = rule.output[42].value;
      //     break;
      //   case DL_EDGE_TOP_LEFT | DL_EDGE_TOP_RIGHT | DL_EDGE_BOTTOM_LEFT:
      //     new_terrain_id = rule.output[43].value;
      //     break;
      //   case DL_EDGE_TOP_LEFT | DL_EDGE_BOTTOM_RIGHT | DL_EDGE_BOTTOM_LEFT:
      //     new_terrain_id = rule.output[44].value;
      //     break;
      //   case DL_EDGE_TOP_LEFT | DL_EDGE_TOP_RIGHT | DL_EDGE_BOTTOM_RIGHT:
      //     new_terrain_id = rule.output[45].value;
      //     break;
      //   case DL_EDGE_TOP_RIGHT | DL_EDGE_BOTTOM_RIGHT | DL_EDGE_BOTTOM_LEFT:
      //     new_terrain_id = rule.output[46].value;
      //     break;
      //   default:
      //     break;
      // }

      if (new_terrain_id == 0)
      {
        spdlog::warn("Could not find a matching tile for bitmask {}", bitmask);
      }

      new_values.terrain = new_terrain_id;
      break;
    }
    default:
      break;
    }

  } while (old_values.terrain != new_values.terrain);

  chunk->tiles.values[z * width * height + y * width + x].terrain = new_values.terrain;

  // Select vegetation
  if (new_values.decoration == 0)
  {
    new_values.decoration = m_select_decoration(terrain_id, x, y, z);
  }

  chunk->tiles.values[z * width * height + y * width + x].decoration = new_values.decoration;
}

int MapGenerator::m_select_decoration(const int terrain_id, const int x, const int y, const int z)
{
  int decoration = 0;

  // TODO: Integrate to the rule system for any terrain
  if (terrain_id != 2)
  {
    return decoration;
  }

  const auto density = vegetation_density[y * width + x];

  if (density < 0.3f)
  {
    return decoration;
  }

  const auto prob = random::get_real();

  if (prob < 0.4f)
  {
    return decoration;
  }

  int plant_small = 0;
  int plant_big = 0;

  const auto type_threshold = vegetation_type[y * width + x];

  if (type_threshold < 0.0f)
  {
    plant_small = 54;
    plant_big = 53;
  }
  else
  {
    plant_small = 49;
    plant_big = 48;
  }

  if (density < 0.4f)
  {
    decoration = plant_small;
  }
  else
  {
    decoration = plant_big;
  }

  return decoration;
}

uint32_t MapGenerator::m_get_bitmask_4_sided(
    const std::vector<int>& terrain, const int x, const int y, const int z, const int neighbor)
{
  const int padded_width = width + m_generation_padding * 2;
  const int padded_height = height + m_generation_padding * 2;
  uint32_t bitmask = 0;

  // Top
  if (y > 0 && terrain[z * padded_width * padded_height + (y - 1) * padded_width + x] == neighbor)
  {
    bitmask |= DL_EDGE_TOP;
  }
  // Right
  if (x < padded_width - 1 && terrain[z * padded_width * padded_height + y * padded_width + x + 1] == neighbor)
  {
    bitmask |= DL_EDGE_RIGHT;
  }
  // Bottom
  if (y < padded_height - 1 && terrain[z * padded_width * padded_height + (y + 1) * padded_width + x] == neighbor)
  {
    bitmask |= DL_EDGE_BOTTOM;
  }
  // Left
  if (x > 0 && terrain[z * padded_width * padded_height + y * padded_width + x - 1] == neighbor)
  {
    bitmask |= DL_EDGE_LEFT;
  }

  return bitmask;
}

uint32_t MapGenerator::m_get_bitmask_8_sided(
    const std::vector<int>& terrain, const int x, const int y, const int z, const int neighbor, const int source)
{
  if (!m_has_neighbor(terrain, x, y, z, neighbor))
  {
    return DL_EDGE_TOP | DL_EDGE_RIGHT | DL_EDGE_BOTTOM | DL_EDGE_LEFT | DL_EDGE_TOP_LEFT | DL_EDGE_TOP_RIGHT |
           DL_EDGE_BOTTOM_RIGHT | DL_EDGE_BOTTOM_LEFT;
  }

  const int padded_width = width + m_generation_padding * 2;
  const int padded_height = height + m_generation_padding * 2;
  uint32_t bitmask = 0;

  // Top
  if (y > 0 && terrain[z * padded_width * padded_height + (y - 1) * padded_width + x] == source)
  {
    bitmask |= DL_EDGE_TOP;
  }
  // Right
  if (x < padded_width - 1 && terrain[z * padded_width * padded_height + y * padded_width + x + 1] == source)
  {
    bitmask |= DL_EDGE_RIGHT;
  }
  // Bottom
  if (y < padded_height - 1 && terrain[z * padded_width * padded_height + (y + 1) * padded_width + x] == source)
  {
    bitmask |= DL_EDGE_BOTTOM;
  }
  // Left
  if (x > 0 && terrain[z * padded_width * padded_height + y * padded_width + x - 1] == source)
  {
    bitmask |= DL_EDGE_LEFT;
  }
  // Top Left
  if (x > 0 && y > 0 && terrain[z * padded_width * padded_height + (y - 1) * padded_width + x - 1] == source)
  {
    bitmask |= DL_EDGE_TOP_LEFT;
  }
  // Top Right
  if (x < padded_width - 1 && y > 0 &&
      terrain[z * padded_width * padded_height + (y - 1) * padded_width + x + 1] == source)
  {
    bitmask |= DL_EDGE_TOP_RIGHT;
  }
  // Bottom Right
  if (x < padded_width - 1 && y < padded_height - 1 &&
      terrain[z * padded_width * padded_height + (y + 1) * padded_width + x + 1] == source)
  {
    bitmask |= DL_EDGE_BOTTOM_RIGHT;
  }
  // Bottom Left
  if (x > 0 && y < padded_height - 1 &&
      terrain[z * padded_width * padded_height + (y + 1) * padded_width + x - 1] == source)
  {
    bitmask |= DL_EDGE_BOTTOM_LEFT;
  }

  if (!(bitmask & DL_EDGE_LEFT) || !(bitmask & DL_EDGE_TOP))
  {
    bitmask &= ~DL_EDGE_TOP_LEFT;
  }
  if (!(bitmask & DL_EDGE_LEFT) || !(bitmask & DL_EDGE_BOTTOM))
  {
    bitmask &= ~DL_EDGE_BOTTOM_LEFT;
  }
  if (!(bitmask & DL_EDGE_RIGHT) || !(bitmask & DL_EDGE_TOP))
  {
    bitmask &= ~DL_EDGE_TOP_RIGHT;
  }
  if (!(bitmask & DL_EDGE_RIGHT) || !(bitmask & DL_EDGE_BOTTOM))
  {
    bitmask &= ~DL_EDGE_BOTTOM_RIGHT;
  }

  return bitmask;
}

bool MapGenerator::m_has_neighbor(
    const std::vector<int>& terrain, const int x, const int y, const int z, const int neighbor)
{
  const int padded_width = width + m_generation_padding * 2;
  const int padded_height = height + m_generation_padding * 2;

  // Top
  if (y > 0 && terrain[z * padded_width * padded_height + (y - 1) * padded_width + x] == neighbor)
  {
    return true;
  }
  // Right
  if (x < padded_width - 1 && terrain[z * padded_width * padded_height + y * padded_width + x + 1] == neighbor)
  {
    return true;
  }
  // Bottom
  if (y < padded_height - 1 && terrain[z * padded_width * padded_height + (y + 1) * padded_width + x] == neighbor)
  {
    return true;
  }
  // Left
  if (x > 0 && terrain[z * padded_width * padded_height + y * padded_width + x - 1] == neighbor)
  {
    return true;
  }
  // Top Left
  if (x > 0 && y > 0 && terrain[z * padded_width * padded_height + (y - 1) * padded_width + x - 1] == neighbor)
  {
    return true;
  }
  // Top Right
  if (x < padded_width - 1 && y > 0 &&
      terrain[z * padded_width * padded_height + (y - 1) * padded_width + x + 1] == neighbor)
  {
    return true;
  }
  // Bottom Right
  if (x < padded_width - 1 && y < padded_height - 1 &&
      terrain[z * padded_width * padded_height + (y + 1) * padded_width + x + 1] == neighbor)
  {
    return true;
  }
  // Bottom Left
  if (x > 0 && y < padded_height - 1 &&
      terrain[z * padded_width * padded_height + (y + 1) * padded_width + x - 1] == neighbor)
  {
    return true;
  }

  return false;
}

}  // namespace dl
