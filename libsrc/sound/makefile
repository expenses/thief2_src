MAKETOP = $(CWD)

PROJECT = tech

%include $(MRC)\env.rc

%include $(MRC)\init.rc

LIBTARGS = sound
EXETARGS = sndtest

sound_OBJS = sndfmt sample mixer lgsndid fade

sound_HDRS = lgsound.h lgsndid.h sndfmt.h
#sound_DEFINES = SND_TEST

sndtest_OBJS = sndtest
sndtest_LIBS = sound dsound winmm
sndtest_COMPILER_LIBS = uuid
sndtest_RC = sndtest

%include $(MRC)\make.rc

