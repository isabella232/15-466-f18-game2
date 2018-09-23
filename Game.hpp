#pragma once

#include <glm/glm.hpp>

struct Game {
	glm::vec2 paddle = glm::vec2(0.0f,-3.0f);
	glm::vec2 ball = glm::vec2(0.0f, 0.0f);
    glm::vec2 ball_velocity = glm::vec2(0.0f,-2.0f);
	//glm::vec2 ball_velocity = glm::vec2(0.0f, 0.0f);

    glm::vec2 paddle_enemy = glm::vec2(0.0f, 3.0f);
    glm::vec2 crosshair = glm::vec2(0.0f, 0.0f);
    glm::vec2 wolf = glm::vec2(0.0f, 0.0f);

	void update(float time);

	static constexpr const float FrameWidth = 10.0f;
	static constexpr const float FrameHeight = 8.0f;
	static constexpr const float PaddleWidth = 2.0f;
	static constexpr const float PaddleHeight = 0.4f;
	static constexpr const float BallRadius = 0.5f;

    struct {
        bool move_left = false;
        bool move_right = false;
        bool move_up = false;
        bool move_down = false;
    } controls;

    struct {
        bool is_hunter = false;
        bool is_wolf = false;
    } identity;

};
