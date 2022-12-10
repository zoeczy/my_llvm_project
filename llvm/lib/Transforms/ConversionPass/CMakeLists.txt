add_llvm_library ( LLVMConversion MODULE BUILDTREE_ONLY
	ConversionPass.cc

	DEPENDS
	intrinsics_gen
	PLUGIN_TOOL
	opt
	)	
