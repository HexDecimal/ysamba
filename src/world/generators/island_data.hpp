#pragma once

#include <cstdint>
#include <utility>
#include "./lib/gal/diagram.hpp"
#include "../point.hpp"

namespace dl
{
  using Edge = std::pair<Point<int>, Point<int>>;
  using Diagram = gal::Diagram<double>;
  using Site = gal::Diagram<double>::Site;
  using GALPoint = gal::Vector2<double>;

  struct IslandData
  {
    struct Structure
    {
      Diagram diagram = Diagram({});
      std::vector<const Site*> coast_sites;
      std::vector<const Site*> land_sites;

      static inline Point<int> expand_point(const GALPoint& point, const int width, const int height)
      {
        return std::move(Point<int>(std::round(point.x * width), std::round(point.y * height)));
      }
    };

    std::vector<Point<int>> points;
    std::vector<int> mask;
    Structure structure;
    Point<int> top_left;
    Point<int> bottom_right;
  };
}
