#pragma once
#include <map>
#include <memory>
#include <string>
#include "type/room_types.h"

class Player;

struct MatchLobbyState {
    std::shared_ptr<Player> waiting;
    std::map<std::string, RoomState> rooms;
    int next_room_id = 1;
};
