using System;
using System.Runtime.InteropServices;

namespace csharp
{
    class Program
    {
        static int Main(string[] args)
        {
            int ret;
            if (args.Length != 1)
            {
                Console.WriteLine("The parameter input is wrong, the usage method is as follows:");
                Console.WriteLine("     dotnet.exe udid");
                ret = 2;
            }
            else
            {
                ret = NativeMethods.aid(args[0]);
            }
            return ret;
        }
    }
}
