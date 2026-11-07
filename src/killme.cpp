/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#ifdef EDITOR
#line 1 "r:\\prj\\thief2\\skup\\thief2\\src\\killme.cpp"
#else
#line 1 "s:\\prj\\thief2\\src\\killme.cpp"
#endif
#line 1
extern "C" int kbd_modifier_state;

extern "C" int hack_for_kbd_state(void)
{
   return kbd_modifier_state;
}
