#include <stdlib.h>
#include <math.h>
#include <hashpp.h>

////////////////////////////////////////////////////////////
// HELPER FUNCTIONS
//

int cHashHelperFunctions::hashlog2(int x)
{
   if (x < 2) return 0;
   return 1+hashlog2(x/2);
}


int cHashHelperFunctions::expmod(int b, int e, uint m)
{
   if (e == 0) return 1;
   if (e%2 == 0)
   {
      int tmp = expmod(b,e/2,m);
      return (tmp*tmp)%m;
   }
   else
   {
      int tmp = expmod(b,e-1,m);
      return (b*tmp)%m;
   }

}



bool cHashHelperFunctions::is_fermat_prime(uint n, uint numtests)
{
   int i;
   if (n < 3) return FALSE;
   for (i = 0; i < numtests; i++)
   {
      int a = rand()%(n-2) + 2;
      if (expmod(a,n,n) != a) return FALSE;
   }
   return TRUE;
}

#define NUM_FERMAT_TESTS 30

bool cHashHelperFunctions::is_prime(uint n)
{
   if (n < 3)
      return FALSE;
   if (n % 2 == 0)
      return FALSE;

   uint sqrt_n = (uint)sqrt((double)n);

   for (uint i = 3; i <= sqrt_n; i += 2)
      if (n % i == 0)
         return FALSE;

   return TRUE;
}


