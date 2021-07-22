#pragma once

namespace Nagi
{

	enum class MoveDirection
	{
		Left, Right, Up, Down, Forward, Backward
	};

class Camera
{
public:
	Camera(float aspectRatio, float fovInDegs, float nearPlane = 0.1f, float farPlane = 1000.f, float moveSpeed = 35.f, const glm::vec3& initialPosition = glm::vec3(0.f, 5.f, 0.f));
	~Camera() = default;

	void rotateCamera(double mouseDx, double mouseDy, float sensitivity);
	void move(MoveDirection direction);


	void setPosition(const glm::vec3& newPosition);
	void setRotation(const glm::vec3& newRotationInDegs);

	glm::mat4 getViewMatrix() const;
	glm::mat4 getProjectionMatrix() const;
	glm::mat4 getViewProjectionMatrix() const;

	const glm::vec3& getPosition() const;
	const glm::vec3& getLookDirection() const;

	void update(float dt);

	// Temporary count to check update frequency of each
	int m_rotateCount = 0;
	int m_updateCount = 0;

private:
	void applyMoveDirection(const glm::vec3& dir);
	void moveInDirection(const glm::vec3& direction, float speed, float dt);

private:
	static constexpr glm::vec3 s_worldUp = glm::vec3(0.f, 1.f, 0.f);
	static constexpr glm::vec3 s_worldRight = glm::vec3(1.f, 0.f, 0.f);
	static constexpr glm::vec3 s_worldForward = glm::vec3(0.f, 0.f, -1.f);

	glm::vec3 m_frameMoveDir;
	glm::vec3 m_worldPosition;

	// Normalized local unit vectors in world space
	glm::vec3 m_localRight;
	glm::vec3 m_localUp;
	glm::vec3 m_localForward;

	float m_camPitch;
	float m_camYaw;
	float m_mouseSpeed;
	float m_moveSpeed;
	float m_fovInDegs;
	float m_aspectRatio;
	float m_nearPlane;
	float m_farPlane;



};

}
