#include "network/ws_listener.h"
#include "network/http_server.h"
#include "service/match_lobby_service.h"
#include "db/database.h"
#include "utils/env_config.h"
#include <boost/asio.hpp>
#include <iostream>

int main() {
    try {
        utils::EnvConfig::load();
        
        std::string db_host = utils::EnvConfig::get("DB_HOST", "localhost");
        std::string db_port = utils::EnvConfig::get("DB_PORT", "5432");
        std::string db_user = utils::EnvConfig::get("DB_USER", "postgres");
        std::string db_pass = utils::EnvConfig::get("DB_PASS", "postgres");
        std::string db_name = utils::EnvConfig::get("DB_NAME", "chess_db");

        db::auto_create(db_host, db_port, db_user, db_pass, db_name);
        
        std::string conn_str = "host=" + db_host + " port=" + db_port + 
                              " user=" + db_user + " password=" + db_pass + 
                              " dbname=" + db_name;
        db::connect(conn_str);
        db::init_schema();

        auto const address = boost::asio::ip::make_address("0.0.0.0");
        unsigned short ws_port = utils::EnvConfig::getPort("WS_PORT", 8080);
        unsigned short http_port = utils::EnvConfig::getPort("HTTP_PORT", 8081);

        boost::asio::io_context ioc{1};
        auto lobby = std::make_shared<MatchLobbyService>();
        g_lobby = lobby;
        std::cout << "[Init] MatchLobbyService created.\n";

        auto ws_listener = std::make_shared<Listener>(ioc, boost::asio::ip::tcp::endpoint{address, ws_port}, lobby);
        ws_listener->run();
        std::cout << "[Init] WebSocket listener started on port " << ws_port << ".\n";

        auto http_listener = std::make_shared<HttpListener>(ioc, boost::asio::ip::tcp::endpoint{address, http_port});
        http_listener->run();
        std::cout << "[Init] HTTP API listener started on port " << http_port << ".\n";

        std::cout << "--- Server is running ---\n";
        ioc.run();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
