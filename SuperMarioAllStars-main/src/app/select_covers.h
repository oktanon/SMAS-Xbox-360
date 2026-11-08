// The launcher's default covers on the Xbox 360: the box art Super Mario All-Stars's own game select shows, taken
// from the player's ROM the first time (nothing of the game ships with the port). A console of its own runs the
// cartridge from power-on, a few milliseconds of it at a time: its title screen (the All-Stars card's cover), Start,
// then each game's box as the select centres it (Right turns the carousel: Super Mario Bros., The Lost Levels,
// Super Mario Bros. 2, Super Mario Bros. 3), cut out of the picture where the box meets its shadow.
#pragma once
#include <cstdint>
#include <memory>
#include <vector>

class Snes;
class SmasGame;

class SelectCovers {
public:
  explicit SelectCovers(const std::vector<uint8_t>& rom);
  ~SelectCovers();
  // Runs the console for about `budgetUs` microseconds. True once done (or given up: failed()).
  bool step(int budgetUs);
  bool failed() const { return failed_; }

  static constexpr int kPics = 5;  // the launcher's cards: smb1, lostlevels, smb2, smb3, allstars (the title)
  struct Pic {
    std::vector<uint32_t> px;  // 0x00RRGGBB
    int w = 0, h = 0;
  };
  Pic pics[kPics];
  // The pictures taken since the last call, as bits (1 << card).
  unsigned takeNew() {
    const unsigned n = new_;
    new_ = 0;
    return n;
  }

private:
  std::unique_ptr<Snes> snes_;
  std::unique_ptr<SmasGame> game_;
  int frame_ = 0;      // since power-on
  int phase_ = 0;      // 0 to the title, 1 Start held, 2.. waiting for game (phase - 2)'s box, settled
  int hold_ = 0;       // frames the current button is still held
  uint16_t buttons_ = 0;
  uint32_t lastHash_ = 0, takenHash_ = 0;
  int still_ = 0;      // frames the select's boxes have stayed as they are
  bool failed_ = false;
  unsigned new_ = 0;
  void runFrame(bool draw);
  uint32_t boxesHash() const;
  bool cutBox(Pic& out) const;
};
