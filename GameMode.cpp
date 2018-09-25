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

#define DEBUG
#ifdef DEBUG
#define dbg_cout(...) std::cout << __VA_ARGS__ << std::endl;
#else
#define dbg_cout(...)
#endif


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
std::map< uint32_t, Scene::Object* > animal_list;
Scene::Transform *cow_transform = nullptr;
Scene::Transform *pig_transform = nullptr;
Scene::Transform *sheep_transform = nullptr;
Scene::Transform *wolf_transform = nullptr;
Scene::Transform *crosshair_transform = nullptr;
Scene *non_const_scene = nullptr;  // non-const scene pointer for delete_transform function

Scene::Camera *camera = nullptr;

Load< Scene > scene(LoadTagDefault, [](){
	Scene *ret = new Scene;
	//load transform hierarchy:
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
    non_const_scene = ret;

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

    // new line
    state.cow.x = cow_transform->position.x;
    state.cow.y = cow_transform->position.y;
    state.pig.x = pig_transform->position.x;
    state.pig.y = pig_transform->position.y;
    state.sheep.x = sheep_transform->position.x;
    state.sheep.y = sheep_transform->position.y;
    state.wolf.x = wolf_transform->position.x;
    state.wolf.y = wolf_transform->position.y;
    state.crosshair.x = crosshair_transform->position.x;
    state.crosshair.y = crosshair_transform->position.y;

    {  // register animal to animal_list
        uint32_t id = 1;
        for (Scene::Object *obj = scene->first_object; obj != nullptr; obj = obj->alloc_next) {
            auto name = obj->transform->name;
            if (name == "Cow" || name == "Pig" || name == "Sheep" || name == "Wolf") {
                animal_list[id++] = obj;
            }
        }

        dbg_cout("Register animal_list");
        for (auto &it : animal_list) {
            dbg_cout("id " << it.first << " name " << it.second->transform->name);
        }
        dbg_cout("");
    }
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

    {  // player controls
        if (evt.type == SDL_KEYDOWN || evt.type == SDL_KEYUP) {
            // both WASD and arrow keys can control crosshair/wolf
            if (evt.key.keysym.scancode == SDL_SCANCODE_UP ||
                evt.key.keysym.scancode == SDL_SCANCODE_W) {
                state.controls.move_up = (evt.type == SDL_KEYDOWN);
                return true;
            } else if (evt.key.keysym.scancode == SDL_SCANCODE_DOWN ||
                       evt.key.keysym.scancode == SDL_SCANCODE_S) {
                state.controls.move_down = (evt.type == SDL_KEYDOWN);
                return true;
            } else if (evt.key.keysym.scancode == SDL_SCANCODE_LEFT ||
                       evt.key.keysym.scancode == SDL_SCANCODE_A) {
                state.controls.move_left = (evt.type == SDL_KEYDOWN);
                return true;
            } else if (evt.key.keysym.scancode == SDL_SCANCODE_RIGHT ||
                       evt.key.keysym.scancode == SDL_SCANCODE_D) {
                state.controls.move_right = (evt.type == SDL_KEYDOWN);
                return true;
            }

            // press space to attack
            if (evt.key.keysym.scancode == SDL_SCANCODE_SPACE && evt.type == SDL_KEYDOWN) {
                if (state.identity.is_hunter) {
                    dbg_cout("Hunter attack");
                    for (auto &a : animal_list) {
                        uint32_t id = a.first;
                        Scene::Object *obj = a.second;
                        Scene::Transform *t = obj->transform;
                        if (glm::distance(state.crosshair, glm::vec2(t->position.x, t->position.y)) < 1.0f) {
                            dbg_cout("Try to kill id " << id << " name "<< t->name);
                            state.try_attack = std::make_pair(true, id);
                            break;
                        }
                    }
                } else if (state.identity.is_wolf) {
                    dbg_cout("Wolf attack");
                    for (auto &a : animal_list) {
                        uint32_t id = a.first;
                        Scene::Object *obj = a.second;
                        Scene::Transform *t = obj->transform;
                        if (t->name == "Wolf") {  // don't check wolf itself
                            continue;
                        }
                        if (glm::distance(state.wolf, glm::vec2(t->position.x, t->position.y)) < 1.0f) {
                            dbg_cout("Try to kill id " << id << " name "<< t->name);
                            state.try_attack = std::make_pair(true, id);
                            break;
                        }
                    }  // end for
                }  // end else if
            }  // end if
        }
    }


	return false;
}

void GameMode::update(float elapsed) {
	state.update(elapsed);

    // send crosshair/wolf position to server when game state is changed
	if (client.connection && state.need_to_send()) {
        // positions
        if (state.identity.is_hunter) {
            // pc: Crosshair Position
            client.connection.send_raw("pc", 2);
            client.connection.send_raw(&state.crosshair.x, sizeof(float));
            client.connection.send_raw(&state.crosshair.y, sizeof(float));
        } else if (state.identity.is_wolf) {
            // pw: Wolf Position
            client.connection.send_raw("pw", 2);
            client.connection.send_raw(&state.wolf.x, sizeof(float));
            client.connection.send_raw(&state.wolf.y, sizeof(float));
        } else {
            std::cout << "no charactor" << std::endl;
        }

        // attack
        if (state.try_attack.first) {
            dbg_cout("send attack target id " << state.try_attack.second);
            client.connection.send_raw("a", 1);
            client.connection.send_raw(&state.try_attack.second, sizeof(uint32_t));
            state.try_attack = std::make_pair(false, 0);  // reset try_attack
        }
	}

	client.poll([&](Connection *c, Connection::Event event) {
		if (event == Connection::OnOpen) {
			//probably won't get this.
		} else if (event == Connection::OnClose) {
			std::cerr << "Lost connection to server." << std::endl;
		} else { assert(event == Connection::OnRecv);
            while (!c->recv_buffer.empty()) {
                // handle identity
                if (!state.identity.is_hunter && !state.identity.is_wolf) {
                    if (c->recv_buffer[0] == 'i') {  // "ih" or "iw"
                        if (c->recv_buffer.size() < 2) {
                            return;
                        } else {
                            if (c->recv_buffer[1] == 'h') {
                                state.identity.is_hunter = true;
                            } else if (c->recv_buffer[1] == 'w') {
                                state.identity.is_wolf = true;
                            }
                            c->recv_buffer.clear();
                        }
                    } else if (*(c->recv_buffer.begin()) == 'p' && c->recv_buffer.size() >= 10) {
                        // ignore any data received before both of two players are registered

                        // probably won't get here because server will not send position data until both
                        // players are registered
                        c->recv_buffer.erase(c->recv_buffer.begin(),
                                             c->recv_buffer.begin() + 10);
                    } else if (*(c->recv_buffer.begin()) == 'a' && c->recv_buffer.size() >= 5) {
                        c->recv_buffer.erase(c->recv_buffer.begin(),
                                             c->recv_buffer.begin() + 5);
                    }
                // handle position update
                } else if (*(c->recv_buffer.begin()) == 'p') {  // [pw/pc][float x][float y]
                    if (c->recv_buffer.size() < 2 * (sizeof(char) + sizeof(float))) {
                        return;
                    } else {
                        if (*(c->recv_buffer.begin() + 1) == 'w' && state.identity.is_hunter) {
                            memcpy(&state.wolf.x, c->recv_buffer.data() + 2, sizeof(float));
                            memcpy(&state.wolf.y, c->recv_buffer.data() + 2 + sizeof(float), sizeof(float));
                        } else if (*(c->recv_buffer.begin() + 1) == 'c' && state.identity.is_wolf) {
                            memcpy(&state.crosshair.x, c->recv_buffer.data() + 2, sizeof(float));
                            memcpy(&state.crosshair.y, c->recv_buffer.data() + 2 + sizeof(float), sizeof(float));
                        }
                        c->recv_buffer.erase(c->recv_buffer.begin(),
                                             c->recv_buffer.begin() + 2 * (sizeof(char) + sizeof(float)));
                    }
                // handle attack event
                } else if (*(c->recv_buffer.begin()) == 'a') {
                    if (c->recv_buffer.size() < 1 + sizeof(uint32_t)) {
                        return;
                    } else {
                        uint32_t target;
                        memcpy(&target, c->recv_buffer.data() + 1, sizeof(uint32_t));
                        // remove from animal_list, scene, living_animal
                        if (!animal_list.empty()) {
                            Scene::Object *obj = animal_list[target];
                            dbg_cout("Receive kill id " << target << " name " << obj->transform->name);
                            non_const_scene->delete_object(obj);
                            animal_list.erase(target);
                        }
                        if (!state.living_animal.empty()) {
                            state.living_animal.erase(target);
                        }
                        c->recv_buffer.erase(c->recv_buffer.begin(),
                                             c->recv_buffer.begin() + 1 + sizeof(uint32_t));
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
