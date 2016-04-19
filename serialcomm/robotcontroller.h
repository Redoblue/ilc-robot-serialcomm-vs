#ifndef ROBOT_CONTROLLER_H
#define ROBOT_CONTROLLER_H

#pragma once

#ifdef DLL_EXPORT
#define DLLAPI __declspec(dllexport)
#else
#define DLLAPI __declspec(dllimport)
#endif

class TRComm;
class Frame;
class WalkFrame;
class LiftFrame;

class DLLAPI RobotController
{
public:
	RobotController();
	~RobotController();

	//speed configuration
	static const float LOW_WALK_SPEED;
	static const float MIDDLE_WALK_SPEED;
	static const float HIGH_WALK_SPEED;
	static const float MAX_WALK_SPEED;
	static const float LOW_ROTATE_SPEED;
	static const float MIDDLE_ROTATE_SPEED;
	static const float HIGH_ROTATE_SPEED;
	static const float MAX_ROTATE_SPEED;
	static const float LOW_LIFT_SPEED;
	static const float MIDDLE_LIFT_SPEED;
	static const float HIGH_LIFT_SPEED;
	static const float MAX_LIFT_SPEED;

	static enum SpeedMode
	{
		LOW = 0,
		MIDDLE,
		HIGH
	};
	static enum RotateDirection
	{
		CLOCKWISE = 0,
		COUNTER_CLOCKWISE
	};
	static enum LiftDirection
	{
		UP,
		DOWN
	};
	static enum LimitSwitchState
	{
		LIMIT_NONE_TOGGLED = 0,			// limit switch state
		LIMIT_TOP_TOGGLED,
		LIMIT_BOTTOM_TOGGLED,
		LIMIT_UNKNOWN_STATE
	};

	static enum OpenPortState
	{
		PORT_OPEN_SUCCESS = 0,
		PORT_OPEN_FAILURE
	};

	OpenPortState InitPort(const int port) const;
	OpenPortState ResetPort(const int port) const;
	void ClosePort() const;

	void SetWalkSpeedMode(SpeedMode mode);
	SpeedMode GetWalkSpeedMode();
	void SetRotateSpeedMode(SpeedMode mode);
	SpeedMode GetRotateSpeedMode();
	void setLiftSpeedMode(SpeedMode mode);
	SpeedMode GetLiftSpeedMode();

	//some funcs only accept normailized data
	void Walk(WalkFrame &walkFrame);
	void Walk(const float normalizedLineSpeed, const float directionAngle, const float normalizedAngularSpeed);	
	void WalkToward(const float directionAngle, const float normalizedLineSpeed = 0.0f);
	void WalkTo(const float directionAngle, const float length, const float normalizedLineSpeed = 0.0f);
	void RotateToward(RotateDirection direction, const float normalizedAngularSpeed = 0.0f);
	void RotateTo(RotateDirection direction, const float angle, const float normalizedAngularSpeed = 0.0f);
	void Lift(LiftFrame &liftFrame);
	void LiftToward(LiftDirection direction, const float normalizedLineSpeed = 0.0f);
	void LiftTo(LiftDirection direction, const float length, const float normalizedLineSpeed = 0.0f);

	void StopWalk();
	void StopRotate();
	void StopLift();
	void StopAll();

	LimitSwitchState GetLimitSwitchState();

private:
	TRComm *m_trComm;
	WalkFrame *m_walkFrame;
	LiftFrame *m_liftFrame;

	SpeedMode m_walkSpeedMode;
	SpeedMode m_rotateSpeedMode;
	SpeedMode m_liftSpeedMode;

private:
	void SendFrame(Frame *frame, const int len);
	float GetWalkSpeed() const;
	float GetRotateSpeed() const;
	float GetLiftSpeed() const;
};

#endif // ROBOT_CONTROLLER_H
