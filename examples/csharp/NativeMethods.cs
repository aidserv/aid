using System.Runtime.InteropServices;

namespace csharp
{
    public static class NativeMethods
    {
        [DllImport("aid.dll")]
        public static extern int aid(string udid);
    }
}
