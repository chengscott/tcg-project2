#pragma once
#include "action.h"
#include "board.h"
#include <algorithm>
#include <array>
#include <map>
#include <random>
#include <sstream>
#include <string>
#include <type_traits>

class agent {
public:
  agent(const std::string &args = "") {
    std::stringstream ss("name=unknown role=unknown " + args);
    for (std::string pair; ss >> pair;) {
      std::string key = pair.substr(0, pair.find('='));
      std::string value = pair.substr(pair.find('=') + 1);
      meta[key] = {value};
    }
  }
  virtual ~agent() {}
  virtual void open_episode(const std::string &flag = "") {}
  virtual void close_episode(const std::string &flag = "") {}
  virtual action take_action(const board &, unsigned) { return action(); }
  virtual bool check_for_win(const board &) { return false; }

public:
  virtual std::string property(const std::string &key) const {
    return meta.at(key);
  }
  virtual void notify(const std::string &msg) {
    meta[msg.substr(0, msg.find('='))] = {msg.substr(msg.find('=') + 1)};
  }
  virtual std::string name() const { return property("name"); }
  virtual std::string role() const { return property("role"); }

protected:
  typedef std::string key;
  struct value {
    std::string value;
    operator std::string() const { return value; }
    template <typename numeric,
              typename = typename std::enable_if<
                  std::is_arithmetic<numeric>::value, numeric>::type>
    operator numeric() const {
      return numeric(std::stod(value));
    }
  };
  std::map<key, value> meta;
};

class random_agent : public agent {
public:
  random_agent(const std::string &args = "") : agent(args) {
    if (meta.find("seed") != meta.end())
      engine.seed(int(meta["seed"]));
  }
  virtual ~random_agent() {}

protected:
  std::default_random_engine engine;
};

template <class _IntType, size_t _Size> class bag_int_distribution {
public:
  bag_int_distribution() { std::iota(std::begin(bag_), std::end(bag_), 1); }
  void reset() { index_ = _Size; }
  _IntType operator()(std::default_random_engine engine) {
    if (index_ == _Size) {
      std::shuffle(std::begin(bag_), std::end(bag_), engine);
      index_ = 0;
    }
    return bag_[index_++];
  }

private:
  std::array<_IntType, _Size> bag_;
  size_t index_ = _Size;
};

/*struct bitbag_int_distribution {
  void reset() { bag = 0; }
  board::tile_t operator()(std::default_random_engine engine) {
    if (bag == 0) {
      std::shuffle(std::begin(index), std::end(index), engine);
      bag = 0b111;
    }
    for (unsigned idx : index) {
      if ((bag >> idx) & 1u) {
        bag &= ~(1u << idx);
        return idx + 1u;
      }
    }
    assert(false);
  }
  unsigned bag;
  std::array<unsigned, 3> index{0u, 1u, 2u};
};*/

/**
 * random environment
 * add a new random tile to an empty cell
 */
class rndenv : public random_agent {
public:
  rndenv(const std::string &args = "")
      : random_agent("name=random role=environment " + args), popup() {}

  action init_action(size_t step) {
    if (step == 0) {
      popup.reset();
      std::shuffle(std::begin(init_space), std::end(init_space), engine);
    }
    board::tile_t tile = popup(engine);
    return action::place(init_space[step], tile);
  }

  virtual action take_action(const board &after, unsigned move_) {
    auto &cur = space[move_];
    std::shuffle(std::begin(cur), std::end(cur), engine);
    for (unsigned pos : cur) {
      if (after(pos) != 0)
        continue;
      board::tile_t tile = popup(engine);
      return action::place(pos, tile);
    }
    return action();
  }

private:
  std::array<unsigned, 16> init_space{0u, 1u, 2u,  3u,  4u,  5u,  6u,  7u,
                                      8u, 9u, 10u, 11u, 12u, 13u, 14u, 15u};
  std::array<unsigned, 4> space[4]{{12u, 13u, 14u, 15u},
                                   {0u, 4u, 8u, 12u},
                                   {0u, 1u, 2u, 3u},
                                   {3u, 7u, 11u, 15u}};
  bag_int_distribution<board::tile_t, 3> popup;
};

/**
 * dummy player
 * select a legal action randomly
 */
class player : public random_agent {
public:
  player(const std::string &args = "")
      : random_agent("name=dummy role=player " + args),
        opcode({0u, 1u, 2u, 3u}) {}

  virtual action take_action(const board &before, unsigned) {
    std::shuffle(std::begin(opcode), std::end(opcode), engine);
    for (unsigned op : opcode) {
      board::reward_t reward = board(before).slide(op);
      if (reward != -1)
        return action::slide(op);
    }
    return action();
  }

private:
  std::array<unsigned, 4> opcode;
};

/**
 * greedy player
 * select a greedy action
 */
class greedy_player : public random_agent {
public:
  greedy_player(const std::string &args = "")
      : random_agent("name=greedy role=player " + args) {}

  virtual action take_action(const board &before, unsigned = 4) {
    board::reward_t reward[4] = {
        board(before).slide(0u), board(before).slide(1u),
        board(before).slide(2u), board(before).slide(3u)};
    board::reward_t *max_reward = std::max_element(reward, reward + 4);
    if (*max_reward != -1) {
      return action::slide(max_reward - reward);
    }
    return action();
  }
};

/**
 * random environment for search
 * add a new random tile to an empty cell
 */
class search_env : public random_agent {
public:
  search_env(const std::string &args = "")
      : random_agent("name=search_env role=environment " + args) {}

  virtual action take_action(const board &after, unsigned move_) {
    auto &cur = space[move_];
    std::shuffle(std::begin(cur), std::end(cur), engine);
    for (unsigned pos : cur) {
      if (after(pos) != 0)
        continue;
      std::shuffle(std::begin(bag), std::end(bag), engine);
      board::tile_t tile = bag[0];
      return action::place(pos, tile);
    }
    return action();
  }

  void reset() { bag = {1, 2, 3}; }

  void remove(unsigned tile) {
    if (bag.empty())
      reset();
    bag.erase(std::remove(std::begin(bag), std::end(bag), tile), std::end(bag));
  }

private:
  std::array<unsigned, 4> space[4]{{12u, 13u, 14u, 15u},
                                   {0u, 4u, 8u, 12u},
                                   {0u, 1u, 2u, 3u},
                                   {3u, 7u, 11u, 15u}};
  std::vector<unsigned> bag;
};

/**
 * deep greedy player
 * select a greedy action by deep search
 */
class deep_greedy_player : public random_agent {
public:
  deep_greedy_player(const std::string &args = "")
      : random_agent("name=deep_greedy role=player " + args) {}

  virtual action take_action(const board &before, unsigned) {
    board::reward_t reward[4] = {}, rew;
    for (size_t op = 0; op < 3; ++op) {
      board cur(before);
      if ((reward[op] = cur.slide(op)) == -1)
        continue;
      env.reset();
      unsigned move_ = op;
      for (size_t depth = 0; depth < 3; ++depth) {
        env.take_action(cur, move_).apply(cur);
        action move = player.take_action(cur);
        rew = move.apply(cur);
        if (rew == -1)
          break;
        reward[op] += rew;
        move_ = move.event() & 0b11;
      }
    }
    board::reward_t *max_reward = std::max_element(reward, reward + 4);
    if (*max_reward != -1) {
      return action::slide(max_reward - reward);
    }
    return action();
  }

private:
  greedy_player player;
  search_env env;
};
