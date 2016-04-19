#pragma once
#ifndef SERIAL_FRAME_H
#define SERIAL_FRAME_H

#include <stdint.h>
#include "robotcontroller.h"

#define PI 3.14f

#ifdef DLL_EXPORT
#define DLLAPI __declspec(dllexport)
#else
#define DLLAPI __declspec(dllimport)
#endif

class DLLAPI Frame
{
public:
	//virtual int FromStream(uint8_t &stream) = 0;
	virtual void ToStream(uint8_t* stream) = 0;

public:
	// frame info
	static const uint8_t FRAME_HEAD_1;
	static const uint8_t FRAME_HEAD_2;
	static const uint8_t FRAME_TAIL;
	// return control
	enum
	{
		CONVERTION_SUCCESS = 0,
		CONVERTION_FAILURE = 1
	};
};

class DLLAPI DataFrame : public Frame
{
public:
	DataFrame();
	~DataFrame();

	//virtual int FromStream(uint8_t &stream);
	virtual void ToStream(uint8_t* stream);

public:
	static const uint8_t FRAME_TYPE;
	static const int FRAME_DATA_NUM;
	static const int FRAME_DATA_LENGTH;
	static const int FRAME_XOR_FLAG;
	static const int FRAME_END_FLAG;
	static const int FRAME_LENGTH;

	RobotController::LimitSwitchState m_limitSwichState;				// limit switch state
};

class DLLAPI WalkFrame : public Frame
{
public:
	WalkFrame();
	WalkFrame(float lineSpeed, float directionAngle, float angularSpeed);
	~WalkFrame();

	//virtual int FromStream(uint8_t &stream);
	virtual void ToStream(uint8_t* stream);

public:
	static const uint8_t FRAME_TYPE;
	static const int FRAME_DATA_NUM;
	static const int FRAME_DATA_LENGTH;
	static const int FRAME_XOR_FLAG;
	static const int FRAME_END_FLAG;
	static const int FRAME_LENGTH;

	float m_lineSpeed;
	float m_directionAngle;
	float m_angularSpeed;
};

class DLLAPI LiftFrame : public Frame
{
public:
	LiftFrame();
	LiftFrame(float lineSpeed);
	~LiftFrame();

	//virtual int FromStream(uint8_t &stream);
	virtual void ToStream(uint8_t* stream);

public:
	static const uint8_t FRAME_TYPE;
	static const int FRAME_DATA_NUM;
	static const int FRAME_DATA_LENGTH;
	static const int FRAME_XOR_FLAG;
	static const int FRAME_END_FLAG;
	static const int FRAME_LENGTH;

	float m_lineSpeed;
};

#endif // SERIAL_FRAME_H
