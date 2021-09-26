#include "Guid.h"
#include "Core.h"
#ifdef _MSC_VER
#include <combaseapi.h>
#else
#include <uuid/uuid.h>
#endif // DEBUG

FGuid::FGuid() {
#ifdef _MSC_VER
    HRESULT Hr = CoCreateGuid((GUID*)this);
    if (!SUCCEEDED(Hr)) {
        TranslateHResult(Hr);
    }
#else
    uuid_generate((unsigned char*)this);
#endif // DEBUG
}


