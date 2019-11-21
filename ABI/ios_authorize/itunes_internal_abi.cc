#include "abi/ios_authorize/itunes_internal_abi.h"
#include "ABI/base/basetypes.h"
#include "abi/ios_authorize/itunes_module.h"

namespace ABI{
	namespace internal{
		namespace iTunesInternalABI{
			int byte_10068BF3 = 0;
			unsigned long GetITunesAddress(){
				iTunes::iTunesModule module;
				unsigned long a1 = reinterpret_cast<unsigned long>(LoadLibraryW(module.iTunesDll(L"iTunes.dll").c_str()));
				HANDLE v1; // eax@1
				BOOL result; // eax@1
				char *v3; // ebp@2
				unsigned __int32 *v4; // esi@2
				signed int v5; // eax@4
				int v6; // ecx@4
				HMODULE v7; // edx@4
				unsigned __int32 *v8; // eax@10
				struct _MODULEINFO modinfo; // [sp+0h] [bp-Ch]@1

				v1 = GetCurrentProcess();
				result = GetModuleInformation(v1, (HMODULE)a1, &modinfo, 0xCu);
				if ( result )
				{
					v3 = (char *)(modinfo.SizeOfImage + a1 - 4096);
					v4 = (unsigned __int32 *)a1;
					if ( a1 >= (unsigned int)v3 )
					{
LABEL_28:
						result = 0;
					}
					else
					{
						do
						{
							if ( *v4 == 0x79736661 )
							{
								v5 = 13;
								v6 = (int)"afsync.rs.sig";
								v7 = reinterpret_cast<HMODULE>(v4);
								while ( *(DWORD *)v7 == *(DWORD *)v6 )
								{
									v5 -= 4;
									v6 += 4;
									++v7;
									if ( (unsigned int)v5 < 4 )
									{
										if ( *(BYTE *)v6 != *(BYTE *)v7 )
											break;
										v8 = (unsigned __int32 *)a1;
										if ( a1 < (unsigned int)v3 )
										{
											while ( 1 )
											{
												if ( (unsigned __int32 *)*v8 == v4 )
												{
													if ( *((BYTE *)v8 - 3) == 0x75 && *((BYTE *)v8 - 0xF) == 0xE8 )
													{
														result = (BOOL)((char *)v8 + *(unsigned __int32 *)((char *)v8 - 0xE) - 10);
														byte_10068BF3 = 0;
														goto LABEL_27;
													}
													if ( *((BYTE *)v8 - 38) == 0xE8 )
													{
														result = (BOOL)((char *)v8 + *(unsigned __int32 *)((char *)v8 - 37) - 33);
														goto LABEL_26;
													}
													if ( *((BYTE *)v8 - 29) == 0xE8 )
													{
														result = (BOOL)((char *)v8 + *(v8 - 7) - 24);
														goto LABEL_26;
													}
													if ( *((BYTE *)v8 - 36) == 0xE8
														&& *((BYTE *)v8 - 10) == 0x6A
														&& *((BYTE *)v8 - 9) == 3
														&& *((BYTE *)v8 - 8) == 0x50 )
														break;
												}
												v8 = (unsigned __int32 *)((char *)v8 + 1);
												if ( (unsigned int)v8 >= (unsigned int)v3 )
													return 0;
											}
											result = (BOOL)((char *)v8 + *(unsigned __int32 *)((char *)v8 - 0x23) - 31);
LABEL_26:
											byte_10068BF3 = 1;
LABEL_27:
											if ( result )
												return result;
										}
										goto LABEL_28;
									}
								}
							}
							v4 = (unsigned __int32 *)((char *)v4 + 1);
						}
						while ( (unsigned int)v4 < (unsigned int)v3 );
						result = 0;
					}
				}
				return result;
			}
		}
	}
}