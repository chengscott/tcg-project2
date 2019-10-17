#pragma once
#include "board.h"
#include <cassert>
#include <iterator>
#include <sstream>
#include <vector>

/**
 * the pattern feature including isomorphism
 *
 * usage:
 *   pattern({ 0, 1, 2, 3 })
 *   pattern({ 0, 1, 2, 3, 4, 5 })
 *
 * isomorphic level of the pattern:
 *   1: no isomorphism
 *   4: enable rotation
 *   8: enable rotation and reflection
 */
class pattern {
public:
  pattern() = default;
  pattern(const std::vector<board::tile_t> &p) : weight_(1 << (p.size() << 2)) {
    size_t psize = p.size();
    assert(psize != 0);
    for (size_t i = 0; i < iso_level_; ++i) {
      board idx(0xfedcba9876543210ull);
      if (i >= 4) {
        idx.mirror();
      }
      idx.rotate_clockwise(i);
      isomorphism[i].reserve(psize);
      for (auto t : p) {
        isomorphism[i].push_back(idx(t));
      }
    }
  }
  pattern(const pattern &) = default;
  pattern(pattern &&) = default;
  pattern &operator=(const pattern &) = default;
  pattern &operator=(pattern &&) = default;
  ~pattern() = default;

public:
  /**
   * estimate the value of a given board
   */
  float estimate(const board &b) const {
    float value = 0;
    for (size_t i = 0; i < iso_level_; ++i) {
      size_t index = indexof(isomorphism[i], b);
      value += weight_[index];
    }
    return value;
  }

  /**
   * update the value of a given board, and return its updated value
   */
  float update(const board &b, float u) {
    float u_split = u / iso_level_;
    float value = 0;
    for (size_t i = 0; i < iso_level_; ++i) {
      size_t index = indexof(isomorphism[i], b);
      weight_[index] += u_split;
      value += weight_[index];
    }
    return value;
  }

private:
  size_t indexof(const std::vector<board::tile_t> &p, const board &b) const {
    size_t index = 0;
    for (size_t i = 0; i < p.size(); ++i)
      index |= b(p[i]) << (i << 2);
    return index;
  }

public:
  std::string nameof(const std::vector<board::tile_t> &p) const {
    std::stringstream ss;
    ss << std::hex;
    std::copy(std::begin(p), std::end(p),
              std::ostream_iterator<board::tile_t>(ss, ""));
    return ss.str();
  }
  std::string name() const {
    return std::to_string(isomorphism[0].size()) + "-tuple pattern " +
           nameof(isomorphism[0]);
  }

  friend std::ostream &operator<<(std::ostream &out, const pattern &p) {
    std::string name = p.name();
    uint32_t len = name.length();
    out.write(reinterpret_cast<char *>(&len), sizeof(len));
    out.write(name.c_str(), len);
    // weight
    uint64_t size = p.weight_.size();
    out.write(reinterpret_cast<const char *>(&size), sizeof(size));
    out.write(reinterpret_cast<const char *>(p.weight_.data()),
              sizeof(float) * size);
    return out;
  }

  friend std::istream &operator>>(std::istream &in, pattern &p) {
    std::string name;
    uint32_t len = 0;
    in.read(reinterpret_cast<char *>(&len), sizeof(len));
    name.resize(len);
    in.read(&name[0], len);
    assert(name == p.name());
    // weight
    uint64_t size = 0;
    in.read(reinterpret_cast<char *>(&size), sizeof(size));
    p.weight_.resize(size);
    in.read(reinterpret_cast<char *>(p.weight_.data()), sizeof(float) * size);
    return in;
  }

private:
  constexpr static const size_t iso_level_ = 8;
  std::array<std::vector<board::tile_t>, iso_level_> isomorphism;
  std::vector<float> weight_;
};