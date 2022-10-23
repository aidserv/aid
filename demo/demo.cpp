#include <iostream>
#include "aid/aiddll.h"

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
		ret = aid(argv[1]);
	}
	return  ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
