#include "network/ws_listener.h"
#include "network/http_server.h"
#include "service/match_lobby_service.h"
#include "db/database.h"
#include "db/migration.h"
#include "utils/env_config.h"
#include <boost/asio.hpp>
#include <iostream>
#include <thread>
#include <chrono>

int main(int argc, char* argv[]) {
    try {
        std::cout.setf(std::ios::unitbuf);
        std::cerr.setf(std::ios::unitbuf);

        if (argc > 1) {
            std::string cmd = argv[1];
            if (cmd == "add-migration") {
                if (argc > 2) {
                    db::migration::add_migration(argv[2]);
                } else {
                    std::cerr << "Usage: server_chinese_chess add-migration <name>\n";
                }
                return 0;
            }
        }

        utils::EnvConfig::load();
        
        std::string db_host = utils::EnvConfig::get("DB_HOST", "localhost");
        std::string db_port = utils::EnvConfig::get("DB_PORT", "5432");
        std::string db_user = utils::EnvConfig::get("DB_USER", "postgres");
        std::string db_pass = utils::EnvConfig::get("DB_PASS", "postgres");
        std::string db_name = utils::EnvConfig::get("DB_NAME", "chess_db");

        std::string conn_str = "host=" + db_host + " port=" + db_port + 
                              " user=" + db_user + " password=" + db_pass + 
                              " dbname=" + db_name;

        int retry = 0;
        bool db_connected = false;
        while (retry < 15) {
            std::cout << "[Init] Attempting to connect to database... (" << retry + 1 << "/15)\n";
            db::auto_create(db_host, db_port, db_user, db_pass, db_name);
            if (db::connect(conn_str)) {
                db_connected = true;
                break;
            }
            std::cout << "[Init] Database connection failed. Retrying in 2 seconds...\n";
            std::this_thread::sleep_for(std::chrono::seconds(2));
            retry++;
        }

        if (!db_connected) {
            throw std::runtime_error("Failed to connect to database after 15 retries.");
        }

        if (argc > 1) {
            std::string cmd = argv[1];
            if (cmd == "update-database") {
                db::migration::update_database();
                return 0;
            }
        }

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
