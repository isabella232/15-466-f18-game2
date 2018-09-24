#include "Connection.hpp"
#include "Game.hpp"

#include <iostream>
#include <set>
#include <chrono>

int main(int argc, char **argv) {
	if (argc != 2) {
		std::cerr << "Usage:\n\t./server <port>" << std::endl;
		return 1;
	}

	Server server(argv[1]);

	Game state;

    bool hunter_is_on = false;
    bool wolf_is_on = false;

	while (1) {
		server.poll([&](Connection *c, Connection::Event evt){
			if (evt == Connection::OnOpen) {
			} else if (evt == Connection::OnClose) {
            } else { assert(evt == Connection::OnRecv);
                if (!hunter_is_on || !wolf_is_on) {
                    // do not send data until two players are registered
                    if (c->recv_buffer[0] == 'h') {
                        c->recv_buffer.clear();
                        if (!hunter_is_on) {
                            c->send("ih");
                            hunter_is_on = true;
                        } else if (!wolf_is_on) {
                            server.connections.back().send("iw");
                            wolf_is_on = true;
                        }
                    } else if (c->recv_buffer[0] == 'p' && c->recv_buffer.size() >= 10) {
                        // ignore any data received before both of two players are registered
                        // e.g. hunter starts sending data once activated
                        c->recv_buffer.erase(c->recv_buffer.begin(),
                                             c->recv_buffer.begin() + 10);
                        c->recv_buffer.shrink_to_fit();
                    }
                } else if (c->recv_buffer[0] == 'p') {
                    // "pc" or "pw" for position of crosshair/wolf
                    if (c->recv_buffer.size() < 2 * (sizeof(char) + sizeof(float))) {
                        return;
                    } else {
                        if (c->recv_buffer[1] == 'c') {
                            memcpy(&state.crosshair.x, c->recv_buffer.data() + 2, sizeof(float));
                            memcpy(&state.crosshair.y, c->recv_buffer.data() + 2 + sizeof(float), sizeof(float));
                            c->recv_buffer.clear();

                            // send crosshair position to wolf
                            if (wolf_is_on) {
                                server.connections.back().send_raw("pc", 2);
                                server.connections.back().send_raw(&state.crosshair.x, sizeof(float));
                                server.connections.back().send_raw(&state.crosshair.y, sizeof(float));
                            }
                        } else if (c->recv_buffer[1] == 'w') {
                            memcpy(&state.wolf.x, c->recv_buffer.data() + 2, sizeof(float));
                            memcpy(&state.wolf.y, c->recv_buffer.data() + 2 + sizeof(float), sizeof(float));
                            c->recv_buffer.clear();

                            // send wolf position to hunter
                            if (hunter_is_on) {
                                server.connections.front().send_raw("pw", 2);
                                server.connections.front().send_raw(&state.wolf.x, sizeof(float));
                                server.connections.front().send_raw(&state.wolf.y, sizeof(float));
                            }
                        }
                    }
                }

			}
        }, 0.01);


		//every second or so, dump the current paddle position:
		static auto then = std::chrono::steady_clock::now();
		auto now = std::chrono::steady_clock::now();
		if (now > then + std::chrono::seconds(1)) {
			then = now;
			std::cout << "Current paddle position: " << state.paddle.x << std::endl;
		}
	}
}
