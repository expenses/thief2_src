MAKETOP = $(CWD)

PROJECT = tech

%include $(MRC)\env.rc

%include $(MRC)\init.rc

LIBTARGS = sound
EXETARGS = sndtest miditest

#sound_OBJS = sndfmt sample mixer lgsndid fade midiplay dsnddynf waildynf timelog
sound_OBJS = sndfmt sample mixer lgsndid fade midiplay dsnddynf waildynf

sound_HDRS = lgsound.h lgsndid.h sndfmt.h midiplay.h timelog.h
#sound_DEFINES = SND_TEST

miditest_OBJS = miditest
miditest_LIBS = sound dsound winmm wail32

sndtest_OBJS = sndtest
sndtest_LIBS = sound dsound winmm wail32
sndtest_COMPILER_LIBS = uuid
sndtest_RC = sndtest

%include $(MRC)\make.rc

