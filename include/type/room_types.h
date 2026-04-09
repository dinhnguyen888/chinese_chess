#pragma once
#include <memory>
#include <string>

class Player;

struct RoomState {
    std::string id;
    std::string name;
    std::shared_ptr<Player> host;
    std::shared_ptr<Player> guest;
    bool is_playing = false;
    bool auto_start = false;
};
