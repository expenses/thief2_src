MAKETOP = $(CWD)

PROJECT = tech

%include $(MRC)\env.rc

%include $(MRC)\init.rc

LIBTARGS = sndutil
EXETARGS = splice stream

sndutil_OBJS = streamer rstremer fstremer splicer rsplicer fsplicer imaadpcm

sndutil_HDRS = sndutil.h
#sndutil_DEFINES = SND_TEST

splice_OBJS = splcmain utilmain
splice_LIBS = lg mprintf range res \
                cpptools comtools compapis appcore templgen sound dsound wail32 sndutil
splice_COMPILER_LIBS = uuid winmm

stream_OBJS = strmmain utilmain
stream_LIBS = lg mprintf range res \
                cpptools comtools compapis appcore templgen sound dsound wail32
stream_COMPILER_LIBS = uuid winmm



%include $(MRC)\make.rc

