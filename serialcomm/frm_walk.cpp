#include "stdafx.h"
#include "serialframe.h"

const uint8_t WalkFrame::FRAME_TYPE = 1;
const int WalkFrame::FRAME_DATA_NUM = 3;
const int WalkFrame::FRAME_DATA_LENGTH = WalkFrame::FRAME_DATA_NUM * sizeof(float);
const int WalkFrame::FRAME_XOR_FLAG = WalkFrame::FRAME_DATA_LENGTH + 3;
const int WalkFrame::FRAME_END_FLAG = WalkFrame::FRAME_XOR_FLAG + 1;
const int WalkFrame::FRAME_LENGTH = WalkFrame::FRAME_END_FLAG + 1;

WalkFrame::WalkFrame()
{
	m_lineSpeed = 0;
	m_directionAngle = PI / 2;
	m_angularSpeed = 0;
}

WalkFrame::WalkFrame(float speed, float directionAngle, float rotationSpeed)
{
	this->m_lineSpeed = speed;
	this->m_directionAngle = directionAngle;
	this->m_angularSpeed = rotationSpeed;
}

WalkFrame::~WalkFrame()
{

}

//int WalkFrame::FromStream(uint8_t &stream)
//{
//
//}

void WalkFrame::ToStream(uint8_t* stream)
{
	assert(stream != NULL);

	stream[0] = WalkFrame::FRAME_HEAD_1;
	stream[1] = WalkFrame::FRAME_HEAD_2;
	stream[2] = WalkFrame::FRAME_TYPE;

	memcpy(&stream[3], &this->m_lineSpeed, sizeof(float));
	memcpy(&stream[7], &this->m_directionAngle, sizeof(float));
	memcpy(&stream[11], &this->m_angularSpeed, sizeof(float));

	stream[WalkFrame::FRAME_XOR_FLAG] = 0;

	for (uint8_t i = 3; i < WalkFrame::FRAME_XOR_FLAG; i++)
		stream[WalkFrame::FRAME_XOR_FLAG] ^= stream[i];

	stream[WalkFrame::FRAME_END_FLAG] = Frame::FRAME_TAIL;
}
