#line 9
#include "loopque.h"
#include <lgassert.h>


void cLoopQueue::PackAppend(const sLoopQueueMessage& message)
{
	if (m_nRemovePoint != 0) {
    	memmove(m_Messages, &m_Messages[m_nRemovePoint], sizeof(sLoopQueueMessage) * (m_nInsertPoint - m_nRemovePoint));
    	m_nInsertPoint -= m_nRemovePoint;
    	m_nRemovePoint = 0;

    	m_Messages[m_nInsertPoint] = message;
    	++m_nInsertPoint;
	} else  {

		CriticalMsg1("Loop queue overflow (size is %d)", kQueueSize);
		return;
	}
}
