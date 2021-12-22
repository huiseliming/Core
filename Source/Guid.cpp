#include "Guid.h"
#include "Core.h"

FGuid::FGuid() 
{
    Data = {};
}

FGuid FGuid::GenerateGuid()
{
    FGuid Guid;
#ifdef _WIN32
    HRESULT Hr = CoCreateGuid((GUID*)&(Guid.Data));
    if (!SUCCEEDED(Hr)) {
        TranslateHResult(Hr);
    }
#else
    uuid_generate((unsigned char*)this);
#endif // DEBUG
    return Guid;
}

std::string FGuid::ToString()
{
    std::string GuidString;
#ifdef _WIN32
    wchar_t GuidWCString[40] = { 0 };
    GuidString.resize(40);
    StringFromGUID2(Data, GuidWCString, 40);
    WideCharToMultiByte(CP_ACP, 0, GuidWCString, -1, GuidString.data(), 40, NULL, NULL);
#else

#endif
    return GuidString;
}


