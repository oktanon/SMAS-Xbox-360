// The Xbox 360's file select (hito 7): between a game's card and the game, its four save files (the cartridge's
// own, A-D, read from the battery save) and its save states, the port's (a slot each, with a picture and a date).
// A file chosen is started through the cartridge's own file panel (SmasGame::requestTitle with the file); a state
// chosen is loaded. The battery save's layout, read in the cartridge's code ($00:8C6E, $00:8D83, the games' save
// routines at $00:8F08, $00:90B5, $00:917F, $00:9239): four files of 0x120 bytes from $0010, each the four games'
// entries one after another (Super Mario Bros. 9 bytes, The Lost Levels 8, Super Mario Bros. 2 9, Super Mario Bros. 3
// 0x106), each entry its data and a 16-bit word that makes the data's byte sum 0; its first byte the world (0-based;
// 0xFF: a new file), The Lost Levels' second its level, Super Mario Bros.' 6th the second quest, its 7th two players.
#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "app/canvas.h"
#include "app/menu.h"
#include "game/smas.h"

struct SaveFileInfo {
  bool used = false;      // (false: a new file)
  std::string where;      // "World 3", "World 4-3"
  bool twoPlayers = false, secondQuest = false;
};
bool readSaveFile(GameId g, int file, const uint8_t* sram, SaveFileInfo* out);
// The entry as the cartridge makes a new one (its defaults, from the ROM: $00:8DE3).
void eraseSaveFile(GameId g, int file, uint8_t* sram, const std::vector<uint8_t>& rom);

class FileSelect {
public:
  static constexpr int kFiles = 4, kSlots = 9;
  enum Action { kNone, kBack, kPlayFile, kLoadState, kEraseFile };
  struct State {
    bool used = false;
    std::string when;          // ("08/11 14:32")
    std::vector<uint32_t> px;  // its picture (none: a state saved before pictures were kept)
    int w = 0, h = 0;
    int image = 0;
  };

  void open(GameId g, int card, const std::string& title, const uint8_t* sram, std::vector<State> states);
  void refreshFiles(const uint8_t* sram);
  void close();
  bool isOpen() const { return open_; }
  GameId game() const { return game_; }
  int card() const { return card_; }
  // `erase`: the erase button's press (the menus' input has no such button).
  Action update(const MenuInput& in, bool erase, float dt);
  int chosen() const { return tab_ == 0 ? file_ : slot_; }  // (the file, 0-3; the slot, 1-9)
  int tab() const { return tab_; }                          // (0 the files, 1 the states)
  // In the menus' style and colour (Menu::skin, its accent). `drawCover`: the card's cover in a rectangle (false:
  // none). `hint`: the buttons' line.
  void draw(Menu& menu, int w, int h, const std::function<bool(RectF)>& drawCover, const std::string& hint);
  void releaseGpu();  // (the canvas's pictures dropped: made again)

private:
  bool open_ = false;
  GameId game_ = GameId::SMB1;
  int card_ = 0;
  std::string title_;
  SaveFileInfo files_[kFiles];
  std::vector<State> states_;
  int tab_ = 0, file_ = 0, slot_ = 1;
  float anim_ = 0, time_ = 0;
  int repeatDir_ = 0;
  float repeatTimer_ = 0;
  Canvas* canvas_ = nullptr;
  unsigned epoch_ = 0;
  void freeImages();
};
