#pragma once

#include "game_object.h"


class BallObject :public GameObject
{
public:
	GLfloat Radius;
	GLboolean Stuck;

	GLboolean Sticky;
	GLboolean PassThrough;

	BallObject();
	BallObject(glm::vec2 pos, GLfloat radius, glm::vec2 velocity, Texture2D sprite);

	glm::vec2 Move(GLfloat dt, GLuint window_width);
	void Reset(glm::vec2 position, glm::vec2 velocity);
};