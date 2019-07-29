#define TEST 0
#define DEBUG 1

#if TEST
#include "test.h"

int main()
{
	testChangeRecord();
	return 0;
}
#endif // TEST

#if DEBUG
#include "searchManage.h"

using namespace quicksearch;

int main()
{
	SearchManage app;
	app.InitSearch();
	app.Search();

	return 0;
}
#endif // DEBUG

