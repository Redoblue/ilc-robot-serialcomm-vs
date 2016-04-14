#define DLL_EXPORT

#include "stdafx.h"
#include "serialframe.h"
#include "robotcontroller.h"

const uint8_t DataFrame::FRAME_TYPE = 0;
const int DataFrame::FRAME_DATA_NUM = 1;
const int DataFrame::FRAME_DATA_LENGTH = DataFrame::FRAME_DATA_NUM * sizeof(int);
const int DataFrame::FRAME_XOR_FLAG = DataFrame::FRAME_DATA_LENGTH + 3;
const int DataFrame::FRAME_END_FLAG = DataFrame::FRAME_XOR_FLAG + 1;
const int DataFrame::FRAME_LENGTH = DataFrame::FRAME_END_FLAG;

DataFrame::DataFrame() :
	m_limitSwichState(RobotController::LimitState::LIMIT_NONE_TOGGLED)
{

}

DataFrame::~DataFrame()
{

}

//int DataFrame::FromStream(uint8_t &stream)
//{
//	
//}

void DataFrame::ToStream(uint8_t* stream)
{

}

