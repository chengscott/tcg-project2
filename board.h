#pragma once
#include <cassert>
#include <iomanip>
#include <iostream>

class board {
public:
  using board_t = uint64_t;
  using row_t = uint16_t;
  using tile_t = uint8_t;
  using reward_t = int;

  board(board_t rhs = 0u) : raw_(rhs) {}
  board(const board &) = default;
  board &operator=(const board &) = default;
  ~board() = default;
  bool operator==(const board &rhs) const { return raw_ == rhs.raw_; }
  bool operator!=(const board &rhs) const { return !(*this == rhs); }

public:
  row_t operator[](size_t i) const { return (raw_ >> (i << 4u)) & 0xffff; }
  tile_t operator()(size_t i) const { return (raw_ >> (i << 2u)) & 0x0f; }
  void set(size_t i, tile_t e) {
    raw_ =
        (raw_ & ~(0x0full << (i << 2u))) | (board_t(e & 0x0full) << (i << 2u));
  }

  reward_t place(size_t pos, tile_t tile) {
    assert(pos < 16 && tile > 0 && tile <= 3);
    set(pos, tile);
    return 0;
  }

  tile_t max_tile() const {
    tile_t ret = 0;
    for (size_t i = 0; i < 16; ++i) {
      tile_t t = operator()(i);
      if (t > ret)
        ret = t;
    }
    return ret;
  }

public:
  reward_t slide(unsigned opcode) {
    switch (opcode & 0b11) {
    case 0:
      return slide_up();
    case 1:
      return slide_right();
    case 2:
      return slide_down();
    case 3:
      return slide_left();
    default:;
    }
    return -1;
  }

private:
  reward_t slide_left() {
    board_t cur = 0u, prev = raw_;
    reward_t score = 0;
    score += lookup::find(operator[](0)).slide_left(cur, 0);
    score += lookup::find(operator[](1)).slide_left(cur, 1);
    score += lookup::find(operator[](2)).slide_left(cur, 2);
    score += lookup::find(operator[](3)).slide_left(cur, 3);
    raw_ = cur;
    return (cur != prev) ? score : -1;
  }
  reward_t slide_right() {
    mirror();
    reward_t score = slide_left();
    mirror();
    return score;
  }
  reward_t slide_up() {
    transpose();
    reward_t score = slide_left();
    mirror();
    transpose();
    flip();
    return score;
  }
  reward_t slide_down() {
    transpose();
    mirror();
    reward_t score = slide_left();
    transpose();
    flip();
    return score;
  }

private:
  /**
   * swap row and column
   * +------------------------+       +------------------------+
   * |     2     8   128     4|       |     2     8     2     4|
   * |     8    32    64   256|       |     8    32     4     2|
   * |     2     4    32   128| ----> |   128    64    32     8|
   * |     4     2     8    16|       |     4   256   128    16|
   * +------------------------+       +------------------------+
   */
  void transpose() {
    raw_ = (raw_ & 0xf0f00f0ff0f00f0full) |
           ((raw_ & 0x0000f0f00000f0f0ull) << 12u) |
           ((raw_ & 0x0f0f00000f0f0000ull) >> 12u);
    raw_ = (raw_ & 0xff00ff0000ff00ffull) |
           ((raw_ & 0x00000000ff00ff00ull) << 24u) |
           ((raw_ & 0x00ff00ff00000000ull) >> 24u);
  }

  /**
   * horizontal reflection
   * +------------------------+       +------------------------+
   * |     2     8   128     4|       |     4   128     8     2|
   * |     8    32    64   256|       |   256    64    32     8|
   * |     2     4    32   128| ----> |   128    32     4     2|
   * |     4     2     8    16|       |    16     8     2     4|
   * +------------------------+       +------------------------+
   */
  void mirror() {
    raw_ = ((raw_ & 0x000f000f000f000full) << 12u) |
           ((raw_ & 0x00f000f000f000f0ull) << 4u) |
           ((raw_ & 0x0f000f000f000f00ull) >> 4u) |
           ((raw_ & 0xf000f000f000f000ull) >> 12u);
  }

  /**
   * vertical reflection
   * +------------------------+       +------------------------+
   * |     2     8   128     4|       |     4     2     8    16|
   * |     8    32    64   256|       |     2     4    32   128|
   * |     2     4    32   128| ----> |     8    32    64   256|
   * |     4     2     8    16|       |     2     8   128     4|
   * +------------------------+       +------------------------+
   */
  void flip() {
    raw_ = ((raw_ & 0x000000000000ffffull) << 48u) |
           ((raw_ & 0x00000000ffff0000ull) << 16u) |
           ((raw_ & 0x0000ffff00000000ull) >> 16u) |
           ((raw_ & 0xffff000000000000ull) >> 48u);
  }

private:
  struct lookup {
    using board_t = board::board_t;
    using row_t = board::row_t;
    using tile_t = board::tile_t;
    using reward_t = board::reward_t;
    lookup() {
      static row_t cur = 0u;
      raw = cur++;
      // left
      left = raw;
      reward_left = mv_left(left);
    }

    static const lookup &find(size_t row) {
      static const lookup cache[65536];
      return cache[row];
    }

    static reward_t mv_left(row_t &row) {
      reward_t reward = 0u;
      tile_t elem[4] = {static_cast<tile_t>((row >> 0u) & 0x0f),
                        static_cast<tile_t>((row >> 4u) & 0x0f),
                        static_cast<tile_t>((row >> 8u) & 0x0f),
                        static_cast<tile_t>((row >> 12u) & 0x0f)};
      size_t m = 4;
      for (size_t c = 0; c < 3; ++c) {
        tile_t rc = elem[c], rcn = elem[c + 1];
        if (rc == 0u && rcn != 0u) {
          m = c;
          elem[c] = rcn;
          break;
        } else if (rc <= 2u && rc + rcn == 3u) {
          m = c;
          elem[c] = 3u;
          reward = 3u;
          break;
        } else if (rc > 2u && rc == rcn) {
          m = c;
          elem[c] = ++rc;
          reward = 3u * (1 << (rc - 3));
          break;
        }
      }
      for (size_t c = m + 1; c < 4; ++c)
        elem[c] = c == 3u ? 0u : elem[c + 1];
      row = ((elem[0] << 0u) | (elem[1] << 4u) | (elem[2] << 8u) |
             (elem[3] << 12u));
      return reward;
    }

    reward_t slide_left(board_t &board, size_t i) const {
      board |= board_t(left) << (i << 4);
      return reward_left;
    }

    row_t raw, left;
    reward_t reward_left;
  };

public:
  friend std::ostream &operator<<(std::ostream &out, const board &b) {
    out << "+------------------------+" << std::endl;
    for (size_t i = 0; i < 4; ++i) {
      out << "|" << std::dec;
      for (size_t j = 0; j < 4; ++j) {
        const tile_t t = b(i * 4 + j);
        out << std::setw(6) << (t <= 3 ? t : ((1 << (t - 1)) - (1 << (t - 3))));
      }
      out << "|" << std::endl;
    }
    out << "+------------------------+" << std::endl;
    return out;
  }

private:
  board_t raw_;
};