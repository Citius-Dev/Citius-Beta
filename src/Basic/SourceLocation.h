#ifndef CITIUS_SOURCE_LOCATION_H
#define CITIUS_SOURCE_LOCATION_H

#include <cstdint>
#include <string>

struct SourceLocation {
  uint32_t Line{0};
  uint32_t Column{0};
  uint32_t Offset{0};
  std::string Filename;

  SourceLocation() = default;

  SourceLocation(uint32_t Line, uint32_t Column, uint32_t Offset,
                 std::string Filename = {})
      : Line(Line), Column(Column), Offset(Offset),
        Filename(std::move(Filename)) {}

  std::string toString() const {
    if (Filename.empty())
      return std::to_string(Line) + ":" + std::to_string(Column);
    return Filename + ":" + std::to_string(Line) + ":" +
           std::to_string(Column);
  }
};

#endif
