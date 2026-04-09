#pragma once
#include <nlohmann/json.hpp>
#include "type/room_types.h"

class RoomService {
public:
    static void broadcast(const RoomState& room, const nlohmann::json& msg);
    static void start_game(RoomState& room);
    static bool is_full(const RoomState& room);
    static nlohmann::json to_json(const RoomState& room);
};
