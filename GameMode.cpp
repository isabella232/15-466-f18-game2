#include "GameMode.hpp"

#include "MenuMode.hpp"
#include "Load.hpp"
#include "MeshBuffer.hpp"
#include "Scene.hpp"
#include "gl_errors.hpp" //helper for dumpping OpenGL error messages
#include "read_chunk.hpp" //helper for reading a vector of structures from a file
#include "data_path.hpp" //helper to get paths relative to executable
#include "compile_program.hpp" //helper to compile opengl shader programs
#include "draw_text.hpp" //helper to... um.. draw text
#include "vertex_color_program.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <map>
#include <cstddef>
#include <random>


Load< MeshBuffer > meshes(LoadTagDefault, [](){
	//return new MeshBuffer(data_path("paddle-ball.pnc"));
	return new MeshBuffer(data_path("wolf_in_sheeps_clothing.pnc"));
});

Load< GLuint > meshes_for_vertex_color_program(LoadTagDefault, [](){
	return new GLuint(meshes->make_vao_for_program(vertex_color_program->program));
});

Scene::Transform *paddle_transform = nullptr;
Scene::Transform *ball_transform = nullptr;
// new line
Scene::Transform *paddle_enemy_transform = nullptr;
Scene::Transform *cow_transform = nullptr;
Scene::Transform *pig_transform = nullptr;
Scene::Transform *sheep_transform = nullptr;
Scene::Transform *wolf_transform = nullptr;
Scene::Transform *crosshair_transform = nullptr;

Scene::Camera *camera = nullptr;

Load< Scene > scene(LoadTagDefault, [](){
	Scene *ret = new Scene;
	//load transform hierarchy:
	//ret->load(data_path("paddle-ball.scene"), [](Scene &s, Scene::Transform *t, std::string const &m){
	ret->load(data_path("wolf_in_sheeps_clothing.scene"), [](Scene &s, Scene::Transform *t, std::string const &m){
		Scene::Object *obj = s.new_object(t);

		obj->program = vertex_color_program->program;
		obj->program_mvp_mat4  = vertex_color_program->object_to_clip_mat4;
		obj->program_mv_mat4x3 = vertex_color_program->object_to_light_mat4x3;
		obj->program_itmv_mat3 = vertex_color_program->normal_to_light_mat3;

		MeshBuffer::Mesh const &mesh = meshes->lookup(m);
		obj->vao = *meshes_for_vertex_color_program;
		obj->start = mesh.start;
		obj->count = mesh.count;
	});

	//look up paddle and ball transforms:
	for (Scene::Transform *t = ret->first_transform; t != nullptr; t = t->alloc_next) {
		if (t->name == "Paddle") {
			if (paddle_transform) throw std::runtime_error("Multiple 'Paddle' transforms in scene.");
			paddle_transform = t;
		}
		if (t->name == "Ball") {
			if (ball_transform) throw std::runtime_error("Multiple 'Ball' transforms in scene.");
			ball_transform = t;
		}
        // new line
		if (t->name == "Cow") {
			if (cow_transform) throw std::runtime_error("Multiple 'Cow' transforms in scene.");
			cow_transform = t;
		}
		if (t->name == "Pig") {
			if (pig_transform) throw std::runtime_error("Multiple 'Pig' transforms in scene.");
			pig_transform = t;
		}
		if (t->name == "Sheep") {
			if (sheep_transform) throw std::runtime_error("Multiple 'Sheep' transforms in scene.");
			sheep_transform = t;
		}
		if (t->name == "Wolf") {
			if (wolf_transform) throw std::runtime_error("Multiple 'Wolf' transforms in scene.");
			wolf_transform = t;
		}
		if (t->name == "Paddle_Enemy") {
			if (paddle_enemy_transform) throw std::runtime_error("Multiple 'Paddle_Enemy' transforms in scene.");
            paddle_enemy_transform = t;
		}
		if (t->name == "Crosshair") {
			if (crosshair_transform) throw std::runtime_error("Multiple 'Crosshair' transforms in scene.");
            crosshair_transform = t;
		}
	}
	if (!paddle_transform) throw std::runtime_error("No 'Paddle' transform in scene.");
	if (!ball_transform) throw std::runtime_error("No 'Ball' transform in scene.");
	if (!cow_transform) throw std::runtime_error("No 'Cow' transform in scene.");
	if (!pig_transform) throw std::runtime_error("No 'Pig' transform in scene.");
	if (!sheep_transform) throw std::runtime_error("No 'Sheep' transform in scene.");
	if (!wolf_transform) throw std::runtime_error("No 'Wolf' transform in scene.");
	if (!crosshair_transform) throw std::runtime_error("No 'Crosshair' transform in scene.");

	//look up the camera:
	for (Scene::Camera *c = ret->first_camera; c != nullptr; c = c->alloc_next) {
		if (c->transform->name == "Camera") {
			if (camera) throw std::runtime_error("Multiple 'Camera' objects in scene.");
			camera = c;
		}
	}
	if (!camera) throw std::runtime_error("No 'Camera' camera in scene.");
	return ret;
});

GameMode::GameMode(Client &client_) : client(client_) {
	client.connection.send_raw("h", 1); //send a 'hello' to the server
}

GameMode::~GameMode() {
}

bool GameMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	//ignore any keys that are the result of automatic key repeat:
	if (evt.type == SDL_KEYDOWN && evt.key.repeat) {
		return false;
	}

	if (evt.type == SDL_MOUSEMOTION) {
		state.paddle.x = (evt.motion.x - 0.5f * window_size.x) / (0.5f * window_size.x) * Game::FrameWidth;
		state.paddle.x = std::max(state.paddle.x, -0.5f * Game::FrameWidth + 0.5f * Game::PaddleWidth);
		state.paddle.x = std::min(state.paddle.x,  0.5f * Game::FrameWidth - 0.5f * Game::PaddleWidth);
	}

    {  // control crosshair
        if (evt.type == SDL_KEYDOWN || evt.type == SDL_KEYUP) {
            //if (evt.key.keysym.scancode == SDL_SCANCODE_W) {
            if (evt.key.keysym.scancode == SDL_SCANCODE_UP) {
                state.controls.move_up = (evt.type == SDL_KEYDOWN);
                return true;
            //} else if (evt.key.keysym.scancode == SDL_SCANCODE_S) {
            } else if (evt.key.keysym.scancode == SDL_SCANCODE_DOWN) {
                state.controls.move_down = (evt.type == SDL_KEYDOWN);
                return true;
            //} else if (evt.key.keysym.scancode == SDL_SCANCODE_A) {
            } else if (evt.key.keysym.scancode == SDL_SCANCODE_LEFT) {
                state.controls.move_left = (evt.type == SDL_KEYDOWN);
                return true;
            //} else if (evt.key.keysym.scancode == SDL_SCANCODE_D) {
            } else if (evt.key.keysym.scancode == SDL_SCANCODE_RIGHT) {
                state.controls.move_right = (evt.type == SDL_KEYDOWN);
                return true;
            }
        }
    }

	return false;
}

void GameMode::update(float elapsed) {
	state.update(elapsed);

    static float accumulate_elapsed = 0.0f;
    accumulate_elapsed += elapsed;

	if (client.connection && accumulate_elapsed > 1.0f/60.0f) {
        accumulate_elapsed = 0.0f;
        // send crosshair/wolf position to server
        if (state.identity.is_hunter) {
            // cp: Crosshair Position
            //std::cout << "hunter send" << std::endl;
            client.connection.send_raw("pc", 2);
            client.connection.send_raw(&state.crosshair.x, sizeof(float));
            client.connection.send_raw(&state.crosshair.y, sizeof(float));
        } else if (state.identity.is_wolf) {
            // wp: Wolf Position
            //std::cout << "wolf send" << std::endl;
            client.connection.send_raw("pw", 2);
            client.connection.send_raw(&state.wolf.x, sizeof(float));
            client.connection.send_raw(&state.wolf.y, sizeof(float));
        } else {
            std::cout << "no charactor" << std::endl;
        }

	}

	client.poll([&](Connection *c, Connection::Event event) {
        //std::cout << "buffer size " << c->recv_buffer.size() << std::endl;
        //std::cout << "Receive msg start with " << c->recv_buffer.at(0) << std::endl;
		if (event == Connection::OnOpen) {
			//probably won't get this.
		} else if (event == Connection::OnClose) {
			std::cerr << "Lost connection to server." << std::endl;
		} else { assert(event == Connection::OnRecv);
            if (!state.identity.is_hunter && !state.identity.is_wolf) {  // no character
                if (c->recv_buffer[0] == 'i') {
                    size_t data_len = 2 * sizeof(char);
                    if (c->recv_buffer.size() < data_len) {
                        return;
                    }
                    else {
                        //std::cout << c->recv_buffer.at(0) << c->recv_buffer.at(1) << c->recv_buffer.at(2) << std::endl;
                        if (c->recv_buffer[1] == 'h') {
                            //std::cout << "set hunter" << std::endl;
                            state.identity.is_hunter = true;
                        }
                        else if (c->recv_buffer[1] == 'w') {
                            //std::cout << "set wolf" << std::endl;
                            state.identity.is_wolf = true;
                        }
                        c->recv_buffer.clear();
                        //std::cout << "cleared buffer size " << c->recv_buffer.size() << std::endl;
                    }
                }
                // receive position data before creating a character, remove data
                else if (c->recv_buffer[0] == 'p' && c->recv_buffer.size() >= 10) {
                    c->recv_buffer.erase(c->recv_buffer.begin(),
                                         c->recv_buffer.begin() + 10);
                    c->recv_buffer.shrink_to_fit();
                }
            }
            else {
                if (c->recv_buffer[0] == 'p') {
                    size_t data_len = 2 * (sizeof(char) + sizeof(float));
                    if (c->recv_buffer.size() < data_len) {
                        return;
                    }
                    else {
                        if (c->recv_buffer[1] == 'w' && state.identity.is_hunter) {
                            //std::cout << "hunter copy wolf data" << std::endl;
                            memcpy(&state.wolf.x, c->recv_buffer.data() + 2, sizeof(float));
                            memcpy(&state.wolf.y, c->recv_buffer.data() + 2 + sizeof(float), sizeof(float));
                        }
                        else if (c->recv_buffer[1] == 'c' && state.identity.is_wolf) {
                            //std::cout << "wolf copy hunter data" << std::endl;
                            memcpy(&state.crosshair.x, c->recv_buffer.data() + 2, sizeof(float));
                            memcpy(&state.crosshair.y, c->recv_buffer.data() + 2 + sizeof(float), sizeof(float));
                        }
                        c->recv_buffer.erase(c->recv_buffer.begin(),
                                             c->recv_buffer.begin() + data_len);
                        c->recv_buffer.shrink_to_fit();
                    }
                }
            }
		}


	});


	//copy game state to scene positions:
	ball_transform->position.x = state.ball.x;
	ball_transform->position.y = state.ball.y;

	paddle_transform->position.x = state.paddle.x;
	paddle_transform->position.y = state.paddle.y;

	paddle_enemy_transform->position.x = state.paddle_enemy.x;
	paddle_enemy_transform->position.y = state.paddle_enemy.y;

    crosshair_transform->position.x = state.crosshair.x;
    crosshair_transform->position.y = state.crosshair.y;

    wolf_transform->position.x = state.wolf.x;
    wolf_transform->position.y = state.wolf.y;
}

void GameMode::draw(glm::uvec2 const &drawable_size) {
	camera->aspect = drawable_size.x / float(drawable_size.y);

	//glClearColor(0.25f, 0.0f, 0.5f, 0.0f);
    glClearColor(136.0f/255.0f, 176.0f/255.0f, 75.0f/255.0f, 0.0);  // grass color
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//set up basic OpenGL state:
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//set up light positions:
	glUseProgram(vertex_color_program->program);

	glUniform3fv(vertex_color_program->sun_color_vec3, 1, glm::value_ptr(glm::vec3(0.81f, 0.81f, 0.76f)));
	glUniform3fv(vertex_color_program->sun_direction_vec3, 1, glm::value_ptr(glm::normalize(glm::vec3(-0.2f, 0.2f, 1.0f))));
	glUniform3fv(vertex_color_program->sky_color_vec3, 1, glm::value_ptr(glm::vec3(0.2f, 0.2f, 0.3f)));
	glUniform3fv(vertex_color_program->sky_direction_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 1.0f, 0.0f)));

	scene->draw(camera);

	GL_ERRORS();
}
