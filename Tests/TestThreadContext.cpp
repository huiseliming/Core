//#define TESTS_OBJECT
#include "Object.h"

int main()
{

#ifdef TESTS_OBJECT

    CObject* O = NewObject<OTest>("dsad");
#endif // DEBUG

    system("pause");
}