#include "Logger.h"

int main()
{
	CLogger log;
	std::vector<std::thread> ts;
	for (size_t i = 0; i < 32; i++)
	{
		ts.push_back(std::thread{
			[&]{
				for (size_t i = 0; i < 100; i++)
				{
					log.Log(ELogLevel::kInfo, std::to_string(i));
				}
			}
		});
	}
	for (size_t i = 0; i < 32; i++)
	{
		ts[i].join();
	}
}




