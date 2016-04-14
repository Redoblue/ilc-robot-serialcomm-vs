#include "serialframe.h"
#include "stdafx.h"

const uint8_t LiftFrame::FRAME_TYPE = 2;
const int LiftFrame::FRAME_DATA_NUM = 1;
const int LiftFrame::FRAME_DATA_LENGTH = LiftFrame::FRAME_DATA_NUM * sizeof(float);
const int LiftFrame::FRAME_XOR_FLAG = LiftFrame::FRAME_DATA_LENGTH + 3;
const int LiftFrame::FRAME_END_FLAG = LiftFrame::FRAME_XOR_FLAG + 1;
const int LiftFrame::FRAME_LENGTH = LiftFrame::FRAME_END_FLAG + 1;

LiftFrame::LiftFrame()
{
	m_lineSpeed = 0;
}

LiftFrame::LiftFrame(float speed)
{
	this->m_lineSpeed = speed;
}

LiftFrame::~LiftFrame()
{

}

//int LiftFrame::FromStream(uint8_t &stream)
//{
//
//}

void LiftFrame::ToStream(uint8_t* stream)
{
	assert(stream != NULL);

	stream[0] = LiftFrame::FRAME_HEAD_1;
	stream[1] = LiftFrame::FRAME_HEAD_2;
	stream[2] = LiftFrame::FRAME_TYPE;

	memcpy(&stream[3], &this->m_lineSpeed, sizeof(float));

	stream[LiftFrame::FRAME_XOR_FLAG] = 0;

	for (uint8_t i = 3; i < LiftFrame::FRAME_XOR_FLAG; i++)
		stream[LiftFrame::FRAME_XOR_FLAG] ^= stream[i];

	stream[LiftFrame::FRAME_END_FLAG] = LiftFrame::FRAME_TAIL;
}
