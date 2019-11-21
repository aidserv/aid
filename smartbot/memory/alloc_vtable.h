#ifndef MEMORY_ALLOC_VTABLE_H_
#define MEMORY_ALLOC_VTABLE_H_
//////////////////////////////////////////////////////////////////////////
namespace memory{
	class DataInterfaceAllocate
	{
	public:
		virtual void DIAllocate() = 0;
		virtual void DIRelease() = 0;
	};
	class DataInterfaceCaller
	{
	public:
		virtual void DICallerAllocate(memory::DataInterfaceAllocate* fmgr) = 0;
		virtual void DICallerRelease(memory::DataInterfaceAllocate* fmgr) = 0;
	};
}
//////////////////////////////////////////////////////////////////////////
#endif