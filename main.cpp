#include <iostream>
#include <memory>
#include <string>
#include <boost/asio.hpp>
#include <nlohmann/json.hpp> // Thư viện nlohmann_json

using boost::asio::ip::tcp;
using json = nlohmann::json;

// Class đại diện cho một phiên kết nối của một client
class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket) : socket_(std::move(socket)) {}

    void start() {
        std::cout << "Client connected: " << socket_.remote_endpoint() << "\n";
        do_read();
    }

private:
    void do_read() {
        auto self(shared_from_this());
        // Lấy dữ liệu từ client, ở đây đọc đến khi có ký tự newline '\n'
        boost::asio::async_read_until(socket_, boost::asio::dynamic_buffer(data_), '\n',
            [this, self](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    std::string message = data_.substr(0, length);
                    data_.erase(0, length);
                    
                    try {
                        // Phân tích cú pháp JSON
                        json req = json::parse(message);
                        std::cout << "Nhận JSON từ Client: " << req.dump(4) << std::endl;
                        
                        // --- Xử lý Game Logic dựa trên dữ liệu JSON ---
                        // Giả sử frontend gửi: {"type": "move", "player": "red", "from": {"x": 1, "y": 2}, "to": {"x":3, "y": 4}}
                        std::string msgType = req.value("type", "unknown");

                        json res;
                        if (msgType == "move") {
                            // Ví dụ về việc cập nhật bàn cờ
                            res["status"] = "success";
                            res["message"] = "Nước đi hợp lệ";
                            res["broad_cast_move"] = req; // Phản hồi lại nước đi để broadcast
                        } else {
                            res["status"] = "unknown";
                            res["message"] = "Không hiểu lệnh";
                        }
                        
                        // Trả về JSON (Lưu ý phải có ký tự \n để Client biết điểm kết thúc của chuỗi dữ liệu)
                        do_write(res.dump() + "\n");
                        
                    } catch (const json::parse_error& e) {
                        std::cerr << "Lỗi Parse JSON: " << e.what() << "\n";
                        
                        // Trả về lỗi
                        json err;
                        err["status"] = "error";
                        err["message"] = "JSON không hợp lệ";
                        do_write(err.dump() + "\n");
                    }
                } else {
                    std::cout << "Client disconnected.\n";
                }
            });
    }

    void do_write(const std::string& message) {
        auto self(shared_from_this());
        boost::asio::async_write(socket_, boost::asio::buffer(message),
            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    // Tiếp tục đợi đọc dữ liệu mới
                    do_read();
                }
            });
    }

    tcp::socket socket_;
    std::string data_;
};

// Class đại diện cho Server lắng nghe kết nối
class Server {
public:
    Server(boost::asio::io_context& io_context, short port)
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
        do_accept();
    }

private:
    void do_accept() {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket) {
                if (!ec) {
                    // Tạo một session mới cho client vừa kết nối
                    std::make_shared<Session>(std::move(socket))->start();
                } else {
                    std::cerr << "Lỗi accept: " << ec.message() << "\n";
                }
                
                // Tiếp tục lắng nghe client tiếp theo
                do_accept();
            });
    }

    tcp::acceptor acceptor_;
};

int main() {
    try {
        boost::asio::io_context io_context;
        // Port server chạy, ví dụ 8080
        short port = 8080;
        
        std::cout << "Khởi chạy Server Cờ Tướng (TCP) trên port " << port << "...\n";
        Server server(io_context, port);

        // Blocking call: Vòng lặp event thực thi các async tasks
        io_context.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}