/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <ixam97@ixam97> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.
 *
 * ----------------------------------------------------------------------------
 * https://github.com/Ixam97
 * ----------------------------------------------------------------------------
 * M‰CAN Control Panel
 * feedbackmonitor.h
 * (c)2022 Maximilian Goldschmidt
 * Commit: [2022-03-10.1]
 */

#pragma once
#include "interface.h"

namespace FeedbackMonitor {
	// Grundlegende Schnittstelle: {
	inline bool b_draw = false;
	void draw();
	void addFrame(Interface::CanFrame& _frame);
	bool getFrame(Interface::CanFrame& _frame);
	// }
};
