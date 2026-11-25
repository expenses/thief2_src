/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/ctagnet.cpp,v 1.6 1999/06/15 13:16:33 Justin Exp $
//
// Networking for the cTags array.
//
// This stuff is pretty closely coupled with ctag.cpp; if you want to
// understand this, make sure you grok that first. This is fairly ugly
// stuff -- ctag wasn't really designed with networking in mind. If we
// ever want to grab some bandwidth efficiency, we might consider
// building the legal tag-table in advance, so it's the same on all
// machines, and none of this mapping stuff is needed...
//
// Assumption: new players don't join in mid-game. If they do, then
// broadcastTagsArray will have to be beefed up to have a way to send
// the array to new players.
//
// @TBD: this is basically all hacked into the side of ctag and ctagset;
// it was designed to be minimally invasive. We could almost certainly do
// something cleaner by restructuring the tags code and putting in a
// "tag manager", which would subsume all of this. However, ctag is used
// by static constructors in other classes, so we have to avoid anything
// that can't happen at static elaboration time...
//
// @NOTE (justin 6/15/99): Tag networking is no longer being used, and
// I've observed a strange hang that occasionally happens at synch time;
// it seems as if DPlay gets hung up in trying to send the tag array.
// So I've disabled tag networking for now.
//

//////////
//
// Utility: growing block
//
// This is more or less ripped out of resstr.
// @TBD: when I get a minute to breathe, unify these and put them into
// cpptools. Make the block size specifiable at construction time.
//

// Tunable parameter: this is the increment we use for allocating the
// string block. Ideally, it should be just a bit larger than we actually
// want the final table to be. But it's only semi-relevant, since this is
// only being used for temporary memory anyway...
#define TAG_STRING_BLOCK_INCREMENT (65536)

// There's gotta be a better way to do this:
class cGrowingBlock {
public:
   cGrowingBlock()
      : m_numIncs(0),
        m_pTempBlock(NULL),
        m_curp(0),
        m_blocksize(0)
   {
      m_pTempBlock = (char *) malloc(TAG_STRING_BLOCK_INCREMENT);
      m_blocksize = TAG_STRING_BLOCK_INCREMENT;
      m_numIncs = 1;
   }

   ~cGrowingBlock()
   {
      free(m_pTempBlock);
   }

   void Append(char c)
   {
      if (m_curp >= m_blocksize) {
         // Grow the block
         m_blocksize += TAG_STRING_BLOCK_INCREMENT;
         char *pNewBlock = (char *) malloc(m_blocksize);
         memcpy(pNewBlock, m_pTempBlock, m_curp);
         free(m_pTempBlock);
         m_pTempBlock = pNewBlock;
      }
      m_pTempBlock[m_curp++] = c;
   }

   // @TBD: this could be more efficient...
   void Append(const char *s)
   {
      while (*s != '\0')
         Append(*s++);
   }

   int GetSize()
   {
      return m_curp;
   }

   char *GetBlock()
   {
      return m_pTempBlock;
   }

private:
   // The current size of the block, in increments:
   int m_numIncs;
   // The temporary holding cell for the strings:
   char *m_pTempBlock;
   // The next index to put a character into (aka the current length):
   int m_curp;
   // The current block size:
   int m_blocksize;
};
