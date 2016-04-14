#define DLL_EXPORT

#include "stdafx.h"
#include "serialframe.h"

const uint8_t Frame::FRAME_HEAD_1 = 0x55;
const uint8_t Frame::FRAME_HEAD_2 = 0xAA;
const uint8_t Frame::FRAME_TAIL = 0x0D;

//int Frame::FromStream(uint8_t &stream) { return 0; };
void Frame::ToStream(uint8_t* stream) { };
