// sdescctl.h
//
// Abstract interface to a control that edits one field of an sdesc'd struct.
// The struct editor dialog lays these out and drives them.
//

#ifndef __SDESCCTL_H
#define __SDESCCTL_H

class cSdescControl
{
public:
   virtual ~cSdescControl() {}; 

   // transfer data to/from the window 
   virtual void Update(BOOL save) = 0; 
   // compute the minimum size of the control
   virtual void MinDims(CWnd* parent, CDC* dc, RECT* rect) = 0; 
   // create the window 
   virtual void Create(CWnd* parent, UINT nID, const RECT* rect) = 0; 
   // destroy the window (and ourselves) 
   virtual void Destroy() = 0; 
   // get the window
   virtual CWnd* Window() = 0; 

   enum 
   {
      kNoFlags = 0, 
      kNoCaption = 1 << 0, // don't make a caption for me
   };
   virtual ulong Flags() { return kNoFlags; }; 
}; 

//
// Base implementation for a control that is also a CWnd 
//

class cBaseSdescControl : public cSdescControl
{
protected:
   CWnd* m_wnd; 

public:
   cBaseSdescControl(CWnd* wnd) : m_wnd(wnd) {};

   void Update(BOOL save) { m_wnd->UpdateData(save); }; 
   void Destroy() { delete m_wnd; }; 
   CWnd* Window() { return m_wnd; };
}; 

//
// Factory signature: build a control for a field in a struct
//

typedef cSdescControl* (*SdescControlFactory)(const struct sFieldDesc* field, void* struc);

#endif // __SDESCCTL_H
