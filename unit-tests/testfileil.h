#include "testfile.h"
struct ILTestConfig
{
	typedef ILFunctionList FunctionList;
	typedef ILVirtualMachineList VirtualMachineList;
	typedef ILFileHelper FileHelper;
	typedef NETArchitecture Architecture;
	typedef NETRuntimeFunctionList RuntimeFunctionList;
	typedef PESegmentList SegmentList;
	typedef NETImportList ImportList;
	typedef NETExportList ExportList;
	typedef BaseFixupList FixupList;
	typedef PESegment Segment;
	typedef BaseFixup Fixup;
	typedef PESectionList SectionList;
	typedef IRelocationList RelocationList;
	typedef NETResourceList ResourceList;
	typedef ISEHandlerList SEHandlerList;
	typedef IImport Import;
	typedef IExport Export;
	typedef PEFile File;
};

typedef TestArchitectureT<ILTestConfig> TestArchitecture;
typedef TestFileT<ILTestConfig> TestFile;
typedef TestSegmentListT<ILTestConfig> TestSegmentList;
typedef TestSegmentT<ILTestConfig> TestSegment;

template<>
inline TestFixupT<ILTestConfig>::TestFixupT(ILTestConfig::FixupList *owner) : ILTestConfig::Fixup(owner) {}
