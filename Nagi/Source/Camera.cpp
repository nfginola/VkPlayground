#include "pch.h"
#include "Camera.h"

namespace Nagi
{
	Camera::Camera(float aspectRatio, float fovInDegs, float nearPlane, float farPlane, float moveSpeed, const glm::vec3& initialPosition) :
		m_frameMoveDir(glm::vec3(0.f)),
		m_worldPosition(initialPosition),
		m_localRight(s_worldRight),
		m_localUp(s_worldUp),
		m_localForward(s_worldForward),
		m_camPitch(0.f),
		m_camYaw(-90.f),		// Positive degrees rotate CCW
		m_mouseSpeed(0.3f),
		m_moveSpeed(moveSpeed),
		m_aspectRatio(aspectRatio),
		m_fovInDegs(fovInDegs),
		m_nearPlane(nearPlane),
		m_farPlane(farPlane)
	{
	}

	void Camera::rotateCamera(double mouseDx, double mouseDy, float dt)
	{
		double deltaYaw = mouseDx * m_mouseSpeed * dt;
		double deltaPitch = mouseDy * m_mouseSpeed * dt;

		m_camYaw += deltaYaw;
		m_camPitch += deltaPitch;

		m_localForward.x = cos(glm::radians(m_camYaw)) * cos(glm::radians(m_camPitch));
		m_localForward.y = sin(glm::radians(m_camPitch));
		m_localForward.z = sin(glm::radians(m_camYaw)) * cos(glm::radians(m_camPitch));

		m_localRight.x = cos(glm::radians(m_camYaw + 90));
		m_localRight.z = sin(glm::radians(m_camYaw + 90));
	}

	void Camera::moveDir(const glm::vec3& dir)
	{
		m_frameMoveDir += dir;
	}

	void Camera::moveDirLeft()
	{
		moveDir(-m_localRight);
	}

	void Camera::moveDirRight()
	{
		moveDir(m_localRight);
	}

	void Camera::moveDirForward()
	{
		moveDir(m_localForward);
	}

	void Camera::moveDirBackward()
	{
		moveDir(-m_localForward);
	}

	void Camera::moveDirUp()
	{
		moveDir(s_worldUp);
	}

	void Camera::moveDirDown()
	{
		moveDir(-s_worldUp);
	}

	void Camera::setPosition(const glm::vec3& newPosition)
	{
		m_worldPosition = newPosition;
	}

	void Camera::setRotation(const glm::vec3& newRotationInDegs)
	{
		//m_rotationInDegs = newRotationInDegs;
	}

	glm::mat4 Camera::getViewMatrix() const
	{
		return glm::lookAtRH(m_worldPosition, m_worldPosition + m_localForward, s_worldUp);
	}

	glm::mat4 Camera::getProjectionMatrix() const
	{
		auto mat = glm::perspectiveRH(glm::radians(m_fovInDegs), m_aspectRatio, m_nearPlane, m_farPlane);
		mat[1][1] *= -1;
		return mat;
	}

	glm::mat4 Camera::getViewProjectionMatrix() const
	{
		return getProjectionMatrix() * getViewMatrix();
	}

	void Camera::update(float dt)
	{
		if (!(glm::length(m_frameMoveDir) <= glm::epsilon<float>()))
			moveInDirection(glm::normalize(m_frameMoveDir), m_moveSpeed, dt);
		m_frameMoveDir = glm::vec3(0.f);
	}

	void Camera::moveInDirection(const glm::vec3& direction, float speed, float dt)
	{
		m_worldPosition += direction * speed * dt;
	}

}