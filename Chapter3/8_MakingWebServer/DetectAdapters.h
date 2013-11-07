#pragma once

#include <vector>

/*

Possible usage example:

   int main()
   {
      std::vector<sAdapterInfo> a;

      Net_EnumerateAdapters( a );

      for(size_t i = 0 ; i < a.size() ; i++)
      {
         printf("[%d] %s; %s\n", i + 1, a[i].FName, a[i].FIP);
      }

      return 0;
   }

*/

/// Short structure with the information about available network adapters
struct sAdapterInfo
{
	char FName[256]; ///< Internal name of the adapter
	char FIP[128];   ///< IP address
	char FID[256];   ///< UID of the adapter
};

bool Net_EnumerateAdapters( std::vector<sAdapterInfo>& Adapters );
