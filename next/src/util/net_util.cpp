#include "multiverso/util/net_util.h"

#include <string>
#include "multiverso/util/log.h"

#ifdef _MSC_VER
#include "winsock2.h"
#include "iphlpapi.h"
#pragma comment(lib, "IPHLPAPI.lib")
#endif

namespace multiverso {
namespace net {

#ifdef _MSC_VER

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

void GetLocalIPAddress(std::unordered_set<std::string>* result) {
  // See MSDN
  // https://msdn.microsoft.com/en-us/library/windows/desktop/aa365917(v=vs.85)

  PIP_ADAPTER_INFO pAdapterInfo;
  PIP_ADAPTER_INFO pAdapter = NULL;
  DWORD dwRetVal = 0;

  ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
  pAdapterInfo = (IP_ADAPTER_INFO *)MALLOC(sizeof(IP_ADAPTER_INFO));
  if (pAdapterInfo == NULL) {
    Log::Fatal("Error allocating memory needed to call GetAdaptersinfo\n");
    return;
  }
  // Make an initial call to GetAdaptersInfo to get
  // the necessary size into the ulOutBufLen variable
  if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
    FREE(pAdapterInfo);
    pAdapterInfo = (IP_ADAPTER_INFO *)MALLOC(ulOutBufLen);
    if (pAdapterInfo == NULL) {
      Log::Fatal("Error allocating memory needed to call GetAdaptersinfo\n");
      return;
    }
  }

  if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR) {
    pAdapter = pAdapterInfo;
    while (pAdapter) {
      // Only care about ETHERNET
      if (pAdapter->Type == MIB_IF_TYPE_ETHERNET) {
        // It is possible for an adapter to have multiple IPv4 addresses,
        IP_ADDR_STRING* pIpAddressList = &pAdapter->IpAddressList;
        while (pIpAddressList) {
          result->insert(pIpAddressList->IpAddress.String);
          pIpAddressList = pIpAddressList->Next;
        }
      }
      pAdapter = pAdapter->Next;
    }
  } else {
    Log::Fatal("GetAdaptersInfo failed with error: %d\n", dwRetVal);
  }
  if (pAdapterInfo)
    FREE(pAdapterInfo);
}

#else

void GetLocalIPAddress(std::unordered_set<std::string>* result) {
CHECK(false);
return;
// todo
}

#endif  // _MSC_VER

}  // namespace net
}  // namespace multiverso
