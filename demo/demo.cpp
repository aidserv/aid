#include <iostream>
#include "ios_cracker/aiddll.h"
#include <Dimm.h>


int main(int argc, char* argv[], char* envp[])
{
	int ret;
	if (argc != 2)
	{
		std::cout << "The parameter input is wrong, the usage method is as follows:\n";
		std::cout << "     " << argv[0] << " udid\n" ;
		ret = 2;
	}
	else
	{
		//while (true)
		//{
		//	//ret = aid("00008110-001271A43462801E");
		//	//std::cout << ret <<std::endl;
		//	//Sleep(2000);

		//	//ret = aid("00008101-0004595C1422001E");
		//	//std::cout << ret <<std::endl;
		//	//Sleep(2000);
		//	ret = aid("d5d0e6c0b6a3fe8ecee1ec8f7f43bac5229e5629");
		//	std::cout << ret <<std::endl;
		//	Sleep(2000);
		//}

		ret = aid(argv[1]);
	}
	return  ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
