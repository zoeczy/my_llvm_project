add_llvm_library ( LLVMPragma MODULE BUILDTREE_ONLY
	PragmaPass.cc

	DEPENDS
	intrinsics_gen
	PLUGIN_TOOL
	opt
	)	
