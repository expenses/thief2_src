// hidden.h
//
// A list of trait categories the user has chosen to hide in the object editor
//

#ifndef __HIDDEN_H
#define __HIDDEN_H

class cHiddenList
{
public:
   cHiddenList() : num(0) { memset(vec,0,sizeof(vec)); };
   ~cHiddenList() { Clear(); }; 

   int Size() { return num; }; 
   const CString& operator[](int n) { return *vec[n]; }; 

   BOOL Hidden(CString cat)
   {
      for (int i = 0; i < num; i++)
         if (cat.CompareNoCase(*vec[i]) == 0)
            return TRUE; 
      return FALSE; 
   }

   void Hide(CString cat)
   {
      if (!Hidden(cat) && num < kVecSize)
         vec[num++] = new CString(cat); 
   }

   void Unhide(CString cat)
   {
      for (int i = 0; i < num; i++)
         if (cat.CompareNoCase(*vec[i]) == 0)
         {
            delete vec[i];
            vec[i] = vec[num-1];
            vec[num-1] = NULL;
            num--;
            return;
         } 
   }

   void Clear()
   {
      for (int i = 0; i < num; i++)
      {
         delete vec[i]; 
         vec[i] = NULL; 
      }
      num = 0; 
   }

protected:
   enum { kVecSize = 64 }; 
   CString* vec[kVecSize]; 
   int num; 
}; 

#endif // __HIDDEN_H
