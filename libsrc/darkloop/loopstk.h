#pragma once

#include <loopapi.h>
#include <looptype.h>

#include <dlist.h>

struct sModeData
{
	sModeData(struct ILoopDispatch* dispatch = NULL, sLoopFrameInfo frame = sLoopFrameInfo(), uint flags = 128)
		: dispatch(dispatch), frame(frame), flags(flags) { }
	sModeData(const sModeData& d);

	ILoopDispatch* dispatch;
	sLoopFrameInfo frame;
	uint flags;
};

class cLoopModeElem : public cDListNode <cLoopModeElem, 1>
{
public:
	cLoopModeElem(sModeData* data)
		: modedata(*data) { }

	sModeData modedata;
};

class cLoopModeStack : public cDList<cLoopModeElem, 1>
{
public:
	cLoopModeStack() { }

	void Push(sModeData data);

	sModeData Pop()
	{
		cLoopModeElem* first = GetFirst();
		if (!first)
			return sModeData();

		sModeData outval = first->modedata;
		Remove(first);
		delete first;
		return outval;
	}

	sModeData* Top()
	{
		return &GetFirst()->modedata;
	}
};