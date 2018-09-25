#include "Connection.hpp"
#include "Game.hpp"

#include <iostream>
#include <set>
#include <chrono>


#define DEBUG
#ifdef DEBUG
#define dbg_cout(...) std::cout << __VA_ARGS__ << std::endl;
#else
#define dbg_cout(...)
#endif

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
                while (!c->recv_buffer.empty()) {
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
                        } else if (*(c->recv_buffer.begin()) == 'p' && c->recv_buffer.size() >= 10) {
                            // ignore any data received before both of two players are registered
                            // e.g. hunter starts sending data once activated
                            c->recv_buffer.erase(c->recv_buffer.begin(),
                                                 c->recv_buffer.begin() + 10);
                        } else if (*(c->recv_buffer.begin()) == 'l' && c->recv_buffer.size() >= 1 + 8) {
                            size_t living_animal_size;
                            memcpy(&living_animal_size, c->recv_buffer.data() + 1, sizeof(size_t));
                            {  // init living_animal
                                for (uint32_t id = 1; id < living_animal_size; id++) {
                                    state.living_animal.insert(id);
                                }
                            }
                            c->recv_buffer.erase(c->recv_buffer.begin(),
                                                 c->recv_buffer.begin() + 1 + 8);
                        }
                    } else if (*(c->recv_buffer.begin()) == 'p') {
                        // "pc" or "pw" for position of crosshair/wolf
                        if (c->recv_buffer.size() < 2 * (sizeof(char) + sizeof(float))) {
                            return;
                        } else {
                            if (*(c->recv_buffer.begin() + 1) == 'c') {
                                memcpy(&state.crosshair.x, c->recv_buffer.data() + 2, sizeof(float));
                                memcpy(&state.crosshair.y, c->recv_buffer.data() + 2 + sizeof(float), sizeof(float));
                                c->recv_buffer.erase(c->recv_buffer.begin(),
                                                     c->recv_buffer.begin() + 10);

                                // send crosshair position to wolf
                                if (wolf_is_on) {
                                    server.connections.back().send_raw("pc", 2);
                                    server.connections.back().send_raw(&state.crosshair.x, sizeof(float));
                                    server.connections.back().send_raw(&state.crosshair.y, sizeof(float));
                                }
                            } else if (*(c->recv_buffer.begin() + 1) == 'w') {
                                memcpy(&state.wolf.x, c->recv_buffer.data() + 2, sizeof(float));
                                memcpy(&state.wolf.y, c->recv_buffer.data() + 2 + sizeof(float), sizeof(float));
                                c->recv_buffer.erase(c->recv_buffer.begin(),
                                                     c->recv_buffer.begin() + 10);

                                // send wolf position to hunter
                                if (hunter_is_on) {
                                    server.connections.front().send_raw("pw", 2);
                                    server.connections.front().send_raw(&state.wolf.x, sizeof(float));
                                    server.connections.front().send_raw(&state.wolf.y, sizeof(float));
                                }
                            }
                        }
                    } else if (*(c->recv_buffer.begin()) == 'a') {
                        if (c->recv_buffer.size() < 1 + sizeof(uint32_t)) {
                            return;
                        } else {
                            uint32_t target;
                            memcpy(&target, c->recv_buffer.data() + 1, sizeof(uint32_t));
                            dbg_cout("Receive attack target id " << target);
                            if (state.living_animal.count(target)) {
                                if (hunter_is_on) {
                                    dbg_cout("send attack target id " << target << " to hunter");
                                    server.connections.front().send_raw("a", 1);
                                    server.connections.front().send_raw(&target, sizeof(uint32_t));
                                }
                                if (wolf_is_on) {
                                    dbg_cout("send attack target id " << target << " to wolf");
                                    server.connections.back().send_raw("a", 1);
                                    server.connections.back().send_raw(&target, sizeof(uint32_t));
                                }
                                state.living_animal.erase(target);
                                dbg_cout("# of living_animal " << state.living_animal.size());
                            }
                            c->recv_buffer.erase(c->recv_buffer.begin(),
                                                 c->recv_buffer.begin() + 1 + sizeof(uint32_t));
                        }
                    } else if (*(c->recv_buffer.begin()) == 'd') {
                        if (c->recv_buffer.size() < 1 + 2 * sizeof(uint32_t)) {
                            return;
                        } else {
                            uint32_t id, direction;
                            memcpy(&id, c->recv_buffer.data() + 1, sizeof(uint32_t));
                            memcpy(&direction, c->recv_buffer.data() + 1 + sizeof(uint32_t), sizeof(uint32_t));
                            if (hunter_is_on) {
                                server.connections.front().send_raw("d", 1);
                                server.connections.front().send_raw(&id, sizeof(float));
                                server.connections.front().send_raw(&direction, sizeof(float));
                            }
                            c->recv_buffer.erase(c->recv_buffer.begin(),
                                                 c->recv_buffer.begin() + 1 + 2 * sizeof(uint32_t));
                        }
                    } else if (*(c->recv_buffer.begin()) == 'c') {  // wolf change skin
                        if (hunter_is_on) {
                            server.connections.front().send_raw("c", 1);
                        }
                        c->recv_buffer.erase(c->recv_buffer.begin(),
                                             c->recv_buffer.begin() + 1);
                    }

                }  // end while
			}  // end else
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
