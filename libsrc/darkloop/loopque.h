#pragma once

#ifdef GetMessage
#undef GetMessage
#endif

#include <looptype.h>

struct sLoopQueueMessage
{
	int message;
	tLoopMessageData hData;
	int flags;
	int alignmentPadTo16Bytes;
};


class cLoopQueue
{
public:
	enum { kQueueSize = 16 };

	cLoopQueue();

	void Append(const sLoopQueueMessage& message);
	BOOL GetMessage(sLoopQueueMessage* pMessage);

	void Flush();
	unsigned CountPending();
	void PackAppend(const sLoopQueueMessage& message);

private:
	ulong m_nRemovePoint;
	ulong m_nInsertPoint;
	sLoopQueueMessage m_Messages[kQueueSize];
};

inline cLoopQueue::cLoopQueue()
{
	m_nRemovePoint = 0;
	m_nInsertPoint = 0;
}
