#include "ios_cracker/apple_auth_device.h"
#pragma comment(lib,"aiddll.lib")
#include <iostream>

int main(int argc, char* argv[], char* envp[])
{
	bool ret;
	if (argc != 2)
	{
		std::cout << "The parameter input is wrong, the usage method is as follows:\n";
		std::cout << "     " << argv[0] << " udid" << endl;
		ret = false;
	}
	else
	{
		ios_cracker::AppleAuthorizeDevice* appAuthDev = new ios_cracker::AppleAuthorizeDevice();
		ret = appAuthDev->AuthorizeMobileDevice("d5d0e6c0b6a3fe8ecee1ec8f7f43bac5229e5629");
		//ret = appAuthDev->AuthorizeMobileDevice("00008110-001271A43462801E");
	}
	return  ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
