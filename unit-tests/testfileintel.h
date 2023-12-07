#include "testfile.h"
struct IntelTestConfig
{
	typedef IntelFunctionList FunctionList;
	typedef IntelVirtualMachineList VirtualMachineList;
	typedef IntelFileHelper FileHelper;
	typedef PEArchitecture Architecture;
	typedef PERuntimeFunctionList RuntimeFunctionList;
	typedef PESegmentList SegmentList;
	typedef PEImportList ImportList;
	typedef PEExportList ExportList;
	typedef PEFixupList FixupList;
	typedef PESegment Segment;
	typedef PEFixup Fixup;
	typedef PESectionList SectionList;
	typedef PERelocationList RelocationList;
	typedef PEResourceList ResourceList;
	typedef PESEHandlerList SEHandlerList;
	typedef PEImport Import;
	typedef PEExport Export;
	typedef PEFile File;
};

typedef TestArchitectureT<IntelTestConfig> TestArchitecture;
typedef TestFileT<IntelTestConfig> TestFile;
typedef TestSegmentListT<IntelTestConfig> TestSegmentList;
typedef TestSegmentT<IntelTestConfig> TestSegment;

template<>
inline TestFixupT<IntelTestConfig>::TestFixupT(PEFixupList *owner) : IntelTestConfig::Fixup(owner, 0, ftUnknown) {}
