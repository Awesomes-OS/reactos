
spec2def(ntdll.dll def/ntdll.spec ADD_IMPORTLIB)

add_definitions(
    -D__NTDLL__
    -D_NTOSKRNL_
    -DCRTDLL)

include_directories(
    BEFORE include
    ${REACTOS_SOURCE_DIR}/sdk/include/reactos/subsys)

list(APPEND SOURCE
    csr/api.c
    csr/capture.c
    csr/connect.c
    dbg/dbgui.c
    ldr/ldrapi.c
    ldr/ldrcor.c
    ldr/ldrfind.c
    ldr/LdrGetDllHandleEx.c
    ldr/LdrGetProcedureAddressForCaller.c
    ldr/LdrImageDirectoryEntryToLoadConfig.c
    ldr/ldrinit.c
    ldr/ldrlock.c
    ldr/ldrmap.c
    ldr/LdrpAddDependency.c
    ldr/LdrpAllocatePlaceHolder.c
    ldr/LdrpApplyFileNameRedirection.c
    ldr/LdrpBuildForwarderLink.c
    ldr/LdrpCompleteMapModule.c
    ldr/LdrpCondenseGraph.c
    ldr/LdrpCreateDllSection.c
    ldr/LdrpDecrementNodeLoadCount.c
    ldr/LdrpDependencyExist.c
    ldr/LdrpDereferenceModule.c
    ldr/LdrpDestroyNode.c
    ldr/LdrpDoPostSnapWork.c
    ldr/LdrpDrainWorkQueue.c
    ldr/LdrpFastpthReloadedDll.c
    ldr/LdrpFindDllActivationContext.c
    ldr/LdrpFindExistingModule.c
    ldr/LdrpFindKnownDll.c
    ldr/LdrpFindLoadedDll.c
    ldr/LdrpFindLoadedDllByAddress.c
    ldr/LdrpFindLoadedDllByHandle.c
    ldr/LdrpFindLoadedDllByName.c
    ldr/LdrpFindLoadedDllByMapping.c
    ldr/LdrpFindLoadedDllByMappingFile.c
    ldr/LdrpFindOrPrepareLoadingModule.c
    ldr/LdrpFreeLoadContext.c
    ldr/LdrpFreeLoadContextOfNode.c
    ldr/LdrpFreeReplacedModule.c
    ldr/LdrpGetDelayloadExportDll.c
    ldr/LdrpGetFullPath.c
    ldr/LdrpGetImportDescriptorForSnap.c
    ldr/LdrpGetNtPathFromDosPath.c
    ldr/LdrpGetProcedureAddress.c
    ldr/LdrpGetShimEngineInterface.c
    ldr/LdrpHandleNewFormatImportDescriptors.c
    ldr/LdrpHandleOldFormatImportDescriptors.c
    ldr/LdrpHandlePendingModuleReplaced.c
    ldr/LdrpHandleReplacedModule.c
    ldr/LdrpHandleTlsData.c
    ldr/LdrpIncrementModuleLoadCount.c
    ldr/LdrpInitializeGraphRecurse.c
    ldr/LdrpInitializeNode.c
    ldr/LdrpInsertMemoryTableEntry.c
    ldr/LdrpIsExtensionPresent.c
    ldr/LdrpLoadContextReplaceModule.c
    ldr/LdrpLoadDependentModule.c
    ldr/LdrpLoadDll.c
    ldr/LdrpLoadDllInternal.c
    ldr/LdrpLoadForwardedDll.c
    ldr/LdrpLoadKnownDll.c
    ldr/LdrpMapAndSnapDependency.c
    ldr/LdrpMapDllFullPath.c
    ldr/LdrpMapDllNtFileName.c
    ldr/LdrpMapDllSearchPath.c
    ldr/LdrpMapDllWithSectionHandle.c
    ldr/LdrpMinimalMapModule.c
    ldr/LdrpNameToOrdinal.c
    ldr/LdrpNotifyLoadOfGraph.c
    ldr/LdrpParseForwarderDescription.c
    ldr/LdrpPinModule.c
    ldr/LdrpPrepareImportAddressTableForSnap.c
    ldr/LdrpPrepareModuleForExecution.c
    ldr/LdrpPreprocessDllName.c
    ldr/LdrpProcessMappedModule.c
    ldr/LdrpProcessWork.c
    ldr/LdrpProtectAndRelocateImage.c
    ldr/LdrpQueueWork.c
    ldr/LdrpRecordModuleDependency.c
    ldr/LdrpRelocateImage.c
    ldr/LdrpReportSnapError.c
    ldr/LdrpResolveDllName.c
    ldr/LdrpResolveForwarder.c
    ldr/LdrpResolveProcedureAddress.c
    ldr/LdrpRunInitializeRoutine.c
    ldr/LdrpSearchPath.c
    ldr/LdrpSetProtection.c
    ldr/LdrpSignalModuleMapped.c
    ldr/LdrpSnapIAT.c
    ldr/LdrpSnapModule.c
    ldr/LdrpSnapThunk.c
    ldr/LdrpTrimTrailingDots.c
    ldr/LdrpUnloadNode.c
    ldr/LdrpUnmapModule.c
    ldr/ldrshim.c
    ldr/ldrsnap.c
    ldr/ldrstring.c
    ldr/ldrtls.c
    ldr/ldrutils.c
    ldr/verifier.c
    rtl/libsupp.c
    rtl/process.c
    rtl/thread.c
    rtl/uilist.c
    rtl/version.c
    etw/trace.c)

if(ARCH STREQUAL "i386")
    list(APPEND ASM_SOURCE dispatch/i386/dispatch.S)
elseif(ARCH STREQUAL "amd64")
    list(APPEND ASM_SOURCE dispatch/amd64/dispatch.S)
elseif(ARCH STREQUAL "arm")
    list(APPEND ASM_SOURCE dispatch/arm/stubs_asm.s)
else()
    list(APPEND SOURCE dispatch/dispatch.c)
endif()

add_asm_files(ntdll_asm ${ASM_SOURCE})

list(APPEND PCH_SKIP_SOURCE
    ${CMAKE_CURRENT_BINARY_DIR}/ntdll_stubs.c)

add_library(ntdll MODULE
    ${SOURCE}
    ${ntdll_asm}
    ${PCH_SKIP_SOURCE}
    def/ntdll.rc
    ${CMAKE_CURRENT_BINARY_DIR}/ntdll.def)

set_module_type(ntdll win32dll ENTRYPOINT 0)
#############################################
## HACK FOR MSVC COMPILATION WITH win32dll ##
set_subsystem(ntdll console)
################# END  HACK #################

if(MSVC)
    add_target_link_flags(ntdll "/RELEASE")
endif()

target_link_libraries(ntdll rtl ntdllsys libcntpr uuid ${PSEH_LIB})
add_pch(ntdll include/ntdll.h "${PCH_SKIP_SOURCE}")
add_dependencies(ntdll ntstatus asm)
add_cd_file(TARGET ntdll DESTINATION reactos/system32 NO_CAB FOR all)
