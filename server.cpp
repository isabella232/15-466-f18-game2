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

	while (1) {
		server.poll([&](Connection *c, Connection::Event evt){
			if (evt == Connection::OnOpen) {
			} else if (evt == Connection::OnClose) {
			} else { assert(evt == Connection::OnRecv);
				if (c->recv_buffer[0] == 'h') {
                    // hello message from the new client
                    // TODO: sent identity msg to client
                    if (server.connections.size() == 1) {
                        // send YOU_ARE_HUNTER message to this client
                        // ih stands for Identity Hunter
                        server.connections.front().send_raw("ih", 2);
                        std::cout << c << "Hello, player 1, you are a hunter." << std::endl;
                    } else if (server.connections.size() == 2) {
                        // send YOU_ARE_WOLF message to this client
                        // iw stands for Identity Wolf
                        server.connections.back().send_raw("iw", 2);
                        std::cout << c << "Hello, player 2, you are a wolf." << std::endl;
                    }
					c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1);
				} else if (c->recv_buffer[0] == 's') {
					if (c->recv_buffer.size() < 1 + sizeof(float)) {
						return; //wait for more data
					} else {
						memcpy(&state.paddle.x, c->recv_buffer.data() + 1, sizeof(float));
						c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1 + sizeof(float));
                        //if (server.connections.size() > 1) {
                            //server.connections.back().send_raw("p", 1);
                            //server.connections.back().send_raw(&state.paddle.x, sizeof(float));
                        //}
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
