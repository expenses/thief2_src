///////////////////////////////////////////////////////////////////////////////
// $Header: r:/prj/cam/src/physics/RCS/phpos.h 1.6 1998/09/18 22:31:01 CCAROLLO Exp $
//
//
//

#ifndef __PHPOS_H
#define __PHPOS_H

#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cPhysPos
//
// Physics location, including ref and back pointer to model
//

// @TBD (toml 08-04-97): remove m_ ?
// @TBD (toml 08-04-97): Naming convention on position and ref?

class cPhysModel;

class cPhysPos
{
public:
   Position     m_position;
   Position     m_endposition;
   Position     m_targetposition;
   cPhysModel * m_pModel;                        // owner model
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__PHPOS_H */
