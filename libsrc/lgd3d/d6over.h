#ifndef __D6OVER_H
#define __D6OVER_H

#include <lgd3d_o.h>

class cD6OverlayHandler;
class cD6AlphaOverlay;

class cD6OvelayType {
public:
    cD6OvelayType():
        m_dwID(0), m_pcParent(NULL),
        m_pcNext(NULL), m_pcPrevious(NULL),
        m_bVisible(0), m_bTurnedOff(0)
    {
    }
    virtual ~cD6OvelayType() {}
    virtual void DrawOverlay() {}

    unsigned int m_dwID;
    cD6OverlayHandler* m_pcParent;
    cD6OvelayType* m_pcNext;
    cD6OvelayType* m_pcPrevious;
    int m_bVisible;
    int m_bTurnedOff;

    void TurnOnOff(int bOn) { m_bTurnedOff = (uint)(bOn == 0); }
};

class cD6OverlayHandler {
public:
    cD6OvelayType m_cListHead;
    sOverRectangle m_viewRect;

    cD6OverlayHandler();
    ~cD6OverlayHandler();
    void AddOverlay(cD6OvelayType* pcNewOver, HLGD3DOVERLAY* phOver);
    void InsertOverlayAfter(cD6OvelayType* pcNewOver, cD6OvelayType* pcAfterOver, HLGD3DOVERLAY* phOver);
    void RemoveOverlay(cD6OvelayType* pcOver);
    void RemoveAllOverlays();
    void KillBranch(cD6OvelayType* pcOver);
    void SetClipViewport(sOverRectangle* psInRect);
    void GetClipViewport(sOverRectangle* psOutRect);
    void DrawOverlays();
    BOOL OverlayFromHandle(HLGD3DOVERLAY hOver, cD6OvelayType** ppcOver);
    BOOL AlphaOverlayFromHandle(HLGD3DOVERLAY hOver, cD6AlphaOverlay** ppcAlphaOver);
    HLGD3DOVERLAY MakeNewOverlayHandle(cD6OvelayType* pcOver);

private:
    static unsigned long s_dwIdGenerator;
};

#endif
