#line 43
#include <lg.h>
#include <windows.h>
#include <cfgdbg.h>
#include <lglog.h>
#include <lgassert.h>
#include <random.h>
#include <dlist.h>
#include <net.h>
#include <msgdelay.h>
#include <string.h>
#include <math.h>

#ifdef NET_ALLOW_SIMULATION
struct cMessage : public cDListNode<cMessage, 1>
{
   ulong m_From;
   ulong m_To;
   ulong m_Flags;
   void *m_Data;
   ulong m_Size;
   ulong m_TimeSent;
   ulong m_Delay;

   cMessage(ulong from, ulong to, ulong flags, void *data, ulong size, ulong timeSent, ulong delay)
      : m_From(from), m_To(to), m_Flags(flags), m_Data(data), m_Size(size), m_TimeSent(timeSent), m_Delay(delay)
   {
   }
};

long double g_ShapeForAverage[50];

cDList<cMessage, 1> g_MessageQueue;

ulong g_MinLatency;
ulong g_AveLatency;
ulong g_MaxLatency;
ulong g_LossPercent;
ulong g_SpikePercent;
ulong g_SpikeLatency;

double __cdecl OldCurvedRand(double shape)
{
   double s = shape;
   double fVar1 = gRNGFibonacci->GetFloat();
   return (1.0 / ((1.0 - s) * fVar1 + s) - 1.0) / (1.0 / s - 1.0);
}

double __cdecl CurvedRand(double shape)
{
   double result = 0.0;
   for (int i = 5; i != 0; i--)
      result += OldCurvedRand(shape);
   return result / 5.0;
}

double __cdecl AverageForShape(double s)
{
   return (s - log(s) - 1.0) / (1.0 - s) * (s / (1.0 - s));
}

void __cdecl ComputeShapeForAverageTable()
{
   double shape = 0.0;
   double delta = 0.0001;
   for (int i = 1; i < 50; i++)
   {
      double average = (double)i / 100.0;
      do
      {
         shape += delta;
         delta += 0.0001;
      }
      while (AverageForShape(shape) < average);
      g_ShapeForAverage[i] = shape;
   }
}

unsigned int __cdecl NextMessageLatency()
{
   if (g_MaxLatency == g_MinLatency)
      return g_MinLatency;

   int averageAsPercent = 100 * (g_AveLatency - g_MinLatency) / (g_MaxLatency - g_MinLatency);
   if (averageAsPercent < 1)
      return g_MinLatency;

   if (averageAsPercent > 49)
   {
      CriticalMsg1("Bad average latency", averageAsPercent);
   }

   long double shape = g_ShapeForAverage[averageAsPercent];
   return (unsigned int)(CurvedRand(shape) * (double)g_MaxLatency + (double)g_MinLatency);
}

void cMessageDelayQueue::DelayMessage(ulong from, ulong to, ulong flags, void *data, ulong size)
{
   int lossFreqInt = (int)(32767.0 * ((double)g_LossPercent / 100.0));

   unsigned int delay = 0;
   if (rand() < lossFreqInt)
   {
      if (!(flags & 1))
      {
         ConfigSpew("net_delay_spew", ("NET DELAY: Dropping packet\n"));
         return;
      }
      delay += NextMessageLatency();
      delay += NextMessageLatency();
   }

   delay += NextMessageLatency();
   unsigned int timeSent = timeGetTime();
   cMessage *pMsg = new cMessage(from, to, flags, data, size, timeSent, delay);
   g_MessageQueue.Append(pMsg);
}

void cMessageDelayQueue::NextReadyMessage(ulong *from, ulong *to, ulong *flags, void **data, ulong *size)
{
   cMessage *pMsg = g_MessageQueue.GetFirst();
   *data = 0;
   unsigned int currentTime = timeGetTime();
   if (pMsg && pMsg->m_TimeSent + pMsg->m_Delay < currentTime)
   {
      ConfigSpew("net_delay_spew", ("NET DELAY: Packet delayed %d, wanted %d\n", currentTime - pMsg->m_TimeSent, pMsg->m_Delay));
      g_MessageQueue.Remove(pMsg);
      *from = pMsg->m_From;
      *to = pMsg->m_To;
      *flags = pMsg->m_Flags;
      *data = pMsg->m_Data;
      *size = pMsg->m_Size;
      delete pMsg;
   }
}

void cMessageDelayQueue::Init()
{
   RandInit(3);
   ComputeShapeForAverageTable();
}

void cMessageDelayQueue::SetFullInternetParameters(ulong lossPercent, ulong minLatency, ulong aveLatency, ulong maxLatency, ulong spikePercent, ulong spikeLatency)
{
   g_MinLatency = minLatency;
   g_AveLatency = aveLatency;
   g_MaxLatency = maxLatency;
   g_SpikeLatency = spikeLatency;
   g_SpikePercent = spikePercent;
   g_LossPercent = lossPercent;
}

void cMessageDelayQueue::GetFullInternetParameters(ulong *lossPercent, ulong *minLatency, ulong *aveLatency, ulong *maxLatency, ulong *spikePercent, ulong *spikeLatency)
{
   *minLatency = g_MinLatency;
   *aveLatency = g_AveLatency;
   *maxLatency = g_MaxLatency;
   *spikeLatency = g_SpikeLatency;
   *spikePercent = g_SpikePercent;
   *lossPercent = g_LossPercent;
}

void cMessageDelayQueue::SetInternetParameters(ulong lossPercent, ulong minLatency, ulong aveLatency, ulong maxLatency)
{
   g_MinLatency = minLatency;
   g_AveLatency = aveLatency;
   g_MaxLatency = maxLatency;
   g_LossPercent = lossPercent;
   g_SpikePercent = 0;
   g_SpikeLatency = 0;
}

void cMessageDelayQueue::GetInternetParameters(ulong *lossPercent, ulong *minLatency, ulong *aveLatency, ulong *maxLatency)
{
   *minLatency = g_MinLatency;
   *aveLatency = g_AveLatency;
   *maxLatency = g_MaxLatency;
   *lossPercent = g_LossPercent;
}

#endif
