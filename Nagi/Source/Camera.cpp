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
		m_camYaw(-90.f),		// Positive degrees rotate CCW		--> Point it at -z
		m_mouseSpeed(1.f),
		m_moveSpeed(moveSpeed),
		m_aspectRatio(aspectRatio),
		m_fovInDegs(fovInDegs),
		m_nearPlane(nearPlane),
		m_farPlane(farPlane)
	{
	}

	void Camera::rotateCamera(double mouseDx, double mouseDy, float sensitivity)
	{
		double deltaYaw = mouseDx * m_mouseSpeed * sensitivity;
		double deltaPitch = mouseDy * m_mouseSpeed * sensitivity;

		m_camYaw += static_cast<float>(deltaYaw);
		m_camPitch += static_cast<float>(deltaPitch);

		// Constrain to avoid gimbal lock
		if (m_camPitch > 89.f)
			m_camPitch = 89.f;
		else if (m_camPitch < -89.f)
			m_camPitch = -89.f;


	}

	void Camera::applyMoveDirection(const glm::vec3& dir)
	{
		m_frameMoveDir += dir;
	}

	void Camera::move(MoveDirection direction)
	{
		switch (direction)
		{
		case MoveDirection::Left:
			applyMoveDirection(-m_localRight);
			break;
		case MoveDirection::Right:
			applyMoveDirection(m_localRight);
			break;
		case MoveDirection::Up:
			applyMoveDirection(s_worldUp);
			break;
		case MoveDirection::Down:
			applyMoveDirection(-s_worldUp);
			break;
		case MoveDirection::Forward:
			applyMoveDirection(m_localForward);
			break;
		case MoveDirection::Backward:
			applyMoveDirection(-m_localForward);
			break;

		default:
			break;
		}
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

	const glm::vec3& Camera::getPosition() const
	{
		return m_worldPosition;
	}

	const glm::vec3& Camera::getLookDirection() const
	{
		return m_localForward;
	}

	void Camera::update(float dt)
	{
		if (!(glm::length(m_frameMoveDir) <= glm::epsilon<float>()))
			moveInDirection(glm::normalize(m_frameMoveDir), m_moveSpeed, dt);
		m_frameMoveDir = glm::vec3(0.f);


		// Spherical coordinates. 
		// We use 2nd factor cos, cos, sin because we are using angles that start on the XZ plane going towards the Y axis, not like traditional Y axis down towards XZ plane
		m_localForward.x = cos(glm::radians(m_camYaw)) * cos(glm::radians(m_camPitch));
		m_localForward.z = sin(glm::radians(m_camYaw)) * cos(glm::radians(m_camPitch));
		m_localForward.y = sin(glm::radians(m_camPitch));

		// We currently dont allow changing the local up vector for the camera. We will just use world up/down
		
		// Change our local right vector (partially, for yaw changes)
		m_localRight.x = cos(glm::radians(m_camYaw + 90));
		m_localRight.z = sin(glm::radians(m_camYaw + 90));

		// Change our local up vector
		// ...

		m_localForward = glm::normalize(m_localForward);
		m_localRight = glm::normalize(m_localRight);
	}

	void Camera::moveInDirection(const glm::vec3& direction, float speed, float dt)
	{
		m_worldPosition += direction * speed * dt;
	}

}