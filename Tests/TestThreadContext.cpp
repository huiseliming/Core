//#define TESTS_OBJECT
#include <string>
int main()
{

#ifdef TESTS_OBJECT

    CObject* O = NewObject<OTest>("dsad");
#endif // DEBUG

    system("pause");
}