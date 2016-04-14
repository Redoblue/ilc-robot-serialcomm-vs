#define DLL_EXPORT

#include "stdafx.h"
#include "robotcontroller.h"
#include "TR_Lib.h"
#include "serialframe.h"

#include <stdio.h>

const float RobotController::LOW_WALK_SPEED = 0.2f;
const float RobotController::MIDDLE_WALK_SPEED = 0.5f;
const float RobotController::HIGH_WALK_SPEED = 0.8f;
const float RobotController::MAX_WALK_SPEED = 1.0f;
const float RobotController::LOW_ROTATE_SPEED = 0.2f;
const float RobotController::MIDDLE_ROTATE_SPEED = 0.5f;
const float RobotController::HIGH_ROTATE_SPEED = 0.8f;
const float RobotController::MAX_ROTATE_SPEED = 1.0f;
const float RobotController::LOW_LIFT_SPEED = 0.2f;
const float RobotController::MIDDLE_LIFT_SPEED = 0.5f;
const float RobotController::HIGH_LIFT_SPEED = 0.8f;
const float RobotController::MAX_LIFT_SPEED = 1.0f;

RobotController::RobotController() :
	m_trComm(new TRComm()),
	m_walkFrame(new WalkFrame()),
	m_liftFrame(new LiftFrame()),
	m_walkSpeedMode(RobotController::SpeedMode::LOW),
	m_liftSpeedMode(RobotController::SpeedMode::LOW),
	m_rotateSpeedMode(RobotController::SpeedMode::LOW)
{

}

RobotController::~RobotController()
{
	delete m_trComm;
	delete m_walkFrame;
}

int RobotController::InitPort(int port) const
{
	if (m_trComm->InitPort(port))
	{
		if (m_trComm->IsOpen())
		{
			m_trComm->StartMonitoring();
		}
		return PORT_OPEN_SUCCESS;
	}
	else
	{
		return PORT_OPEN_FAILURE;
	}
}

int RobotController::ResetPort(int port) const
{
	if (m_trComm->IsOpen())
	{
		m_trComm->RestartMonitoring();
		return PORT_OPEN_SUCCESS;
	}
	else
	{
		if (this->InitPort(port))
		{
			return PORT_OPEN_SUCCESS;
		}
		else
		{
			return PORT_OPEN_FAILURE;
		}
	}
}

void RobotController::ClosePort() const
{
	m_trComm->ClosePort();
}

void RobotController::SetWalkSpeedMode(SpeedMode mode)
{
	m_walkSpeedMode = mode;
}

RobotController::SpeedMode RobotController::GetWalkSpeedMode()
{
	return m_walkSpeedMode;
}

void RobotController::SetRotateSpeedMode(SpeedMode mode)
{
	m_rotateSpeedMode = mode;
}

RobotController::SpeedMode RobotController::GetRotateSpeedMode()
{
	return m_rotateSpeedMode;
}

void RobotController::setLiftSpeedMode(SpeedMode mode)
{
	m_liftSpeedMode = mode;
}

RobotController::SpeedMode RobotController::GetLiftSpeedMode()
{
	return m_liftSpeedMode;
}

void RobotController::Walk(WalkFrame &walkFrame)
{
	SendFrame(&walkFrame, WalkFrame::FRAME_LENGTH);
}

void RobotController::Walk(const float normalizedLineSpeed, const float directionAngle, const float normalizedAngularSpeed)
{
	m_walkFrame->m_lineSpeed = normalizedLineSpeed * MAX_WALK_SPEED;
	m_walkFrame->m_directionAngle = directionAngle;
	m_walkFrame->m_angularSpeed = normalizedAngularSpeed * MAX_ROTATE_SPEED;

	SendFrame(m_walkFrame, WalkFrame::FRAME_LENGTH);
}

void RobotController::WalkToward(const float directionAngle, const float normalizedLineSpeed)
{
	m_walkFrame->m_lineSpeed = normalizedLineSpeed < 0.001f ? GetWalkSpeed() : normalizedLineSpeed * MAX_WALK_SPEED;
	m_walkFrame->m_directionAngle = directionAngle;

	SendFrame(m_walkFrame, WalkFrame::FRAME_LENGTH);
}

void RobotController::WalkTo(const float directionAngle, const float length, const float normalizedLineSpeed)
{
	//rotate to the direction
	StopRotate();
	StopWalk();

	m_walkFrame->m_angularSpeed = GetRotateSpeed();
	SendFrame(m_walkFrame, WalkFrame::FRAME_LENGTH);
	Sleep(1000 * (directionAngle / m_walkFrame->m_angularSpeed));
	StopRotate();

	//walk forward
	m_walkFrame->m_lineSpeed = normalizedLineSpeed < 0.001f ? GetWalkSpeed() : normalizedLineSpeed * MAX_WALK_SPEED;
	m_walkFrame->m_directionAngle = PI / 2;
	SendFrame(m_walkFrame, WalkFrame::FRAME_LENGTH);
	Sleep(1000 * (length / m_walkFrame->m_lineSpeed));
	StopWalk();
}

void RobotController::RotateToward(RotateDirection direction, const float normalizedAngularSpeed)
{
	m_walkFrame->m_angularSpeed = ((normalizedAngularSpeed < 0.001f) ? GetRotateSpeed() : normalizedAngularSpeed * MAX_ROTATE_SPEED);
	m_walkFrame->m_angularSpeed *= ((direction == RotateDirection::CLOCKWISE) ? 1 : -1);

	SendFrame(m_walkFrame, WalkFrame::FRAME_LENGTH);
}

void RobotController::RotateTo(RotateDirection direction, const float angle, const float normalizedAngularSpeed)
{
	m_walkFrame->m_angularSpeed = ((normalizedAngularSpeed < 0.001f) ? GetRotateSpeed() : normalizedAngularSpeed * MAX_ROTATE_SPEED);
	m_walkFrame->m_angularSpeed *= ((direction == RotateDirection::CLOCKWISE) ? 1 : -1);

	SendFrame(m_walkFrame, WalkFrame::FRAME_LENGTH);
	Sleep(1000 * (abs(angle) / abs(m_walkFrame->m_directionAngle)));
	StopRotate();
}

void RobotController::Lift(LiftFrame &liftFrame)
{
	SendFrame(&liftFrame, LiftFrame::FRAME_LENGTH);
}

void RobotController::LiftToward(LiftDirection direction, const float normalizedLineSpeed)
{
	m_liftFrame->m_lineSpeed = ((normalizedLineSpeed < 0.001f) ? GetLiftSpeed() : normalizedLineSpeed * MAX_LIFT_SPEED);
	m_liftFrame->m_lineSpeed *= ((direction == RotateDirection::CLOCKWISE) ? 1 : -1);

	SendFrame(m_liftFrame, LiftFrame::FRAME_LENGTH);
}

void RobotController::LiftTo(LiftDirection direction, const float length, const float normalizedLineSpeed)
{
	m_liftFrame->m_lineSpeed = ((normalizedLineSpeed < 0.001f) ? GetLiftSpeed() : normalizedLineSpeed * MAX_LIFT_SPEED);
	m_liftFrame->m_lineSpeed *= ((direction == LiftDirection::UP) ? 1 : -1);

	SendFrame(m_liftFrame, LiftFrame::FRAME_LENGTH);
	Sleep(1000 * (abs(length) / abs(m_liftFrame->m_lineSpeed)));
	StopLift();
}

void RobotController::StopWalk()
{
	m_walkFrame->m_lineSpeed = 0.0f;
	SendFrame(m_walkFrame, WalkFrame::FRAME_LENGTH);
}

void RobotController::StopRotate()
{
	m_walkFrame->m_angularSpeed = 0.0f;
	SendFrame(m_walkFrame, WalkFrame::FRAME_LENGTH);
}

void RobotController::StopLift()
{
	m_liftFrame->m_lineSpeed = 0.0f;
	SendFrame(m_liftFrame, LiftFrame::FRAME_LENGTH);
}

void RobotController::StopAll()
{
	StopWalk();
	StopRotate();
	StopLift();
}

RobotController::LimitState RobotController::GetSwitchState()
{
	return m_trComm->m_dataFrame->m_limitSwichState;
}

void RobotController::SendFrame(Frame *frame, const int len)
{
	uint8_t* stream = new uint8_t[len];
	frame->ToStream(stream);
	m_trComm->WriteToPort(stream, len);

	// TODO
	for (int i = 0; i < len; i++)
	{
		printf("%x ", stream[i]);
	}
	printf("\n");
	//

	delete[] stream;
}

float RobotController::GetWalkSpeed() const
{
	switch (m_walkSpeedMode)
	{
	case SpeedMode::LOW:
		return LOW_WALK_SPEED;
		break;
	case SpeedMode::MIDDLE:
		return MIDDLE_WALK_SPEED;
		break;
	case SpeedMode::HIGH:
		return HIGH_WALK_SPEED;
		break;
	default:
		return LOW_WALK_SPEED;
		break;
	}
}

float RobotController::GetRotateSpeed() const
{
	switch (m_rotateSpeedMode)
	{
	case SpeedMode::LOW:
		return LOW_ROTATE_SPEED;
		break;
	case SpeedMode::MIDDLE:
		return MIDDLE_ROTATE_SPEED;
		break;
	case SpeedMode::HIGH:
		return HIGH_ROTATE_SPEED;
		break;
	default:
		return LOW_ROTATE_SPEED;
		break;
	}
}

float RobotController::GetLiftSpeed() const
{
	switch (m_liftSpeedMode)
	{
	case SpeedMode::LOW:
		return LOW_LIFT_SPEED;
		break;
	case SpeedMode::MIDDLE:
		return MIDDLE_LIFT_SPEED;
		break;
	case SpeedMode::HIGH:
		return HIGH_LIFT_SPEED;
		break;
	default:
		return LOW_LIFT_SPEED;
		break;
	}
}
