
add_subdirectory(cmlib)
add_subdirectory(inflib)

if(CMAKE_CROSSCOMPILING)

if (NOT NTDLL_WORK)
add_subdirectory(3rdparty)
add_subdirectory(comsupp)
add_subdirectory(conutils)
add_subdirectory(cportlib)
endif()
add_subdirectory(crt)
if (NOT NTDLL_WORK)
add_subdirectory(cryptlib)

if(MSVC)
    add_subdirectory(cpprt)
    add_subdirectory(RunTmChk)
endif()

add_subdirectory(delayimp)
add_subdirectory(dmilib)
#add_subdirectory(dnslib) Nothing links to this lib.
add_subdirectory(drivers)
add_subdirectory(dxguid)
add_subdirectory(epsapi)
add_subdirectory(evtlib)
add_subdirectory(fast486)
add_subdirectory(fslib)

if(STACK_PROTECTOR)
    add_subdirectory(gcc_ssp)
endif()

add_subdirectory(ioevent)
add_subdirectory(lsalib)
add_subdirectory(nt)
add_subdirectory(ppcmmu)
endif()
add_subdirectory(pseh)

if (NOT NTDLL_WORK)
if(KDBG)
    add_subdirectory(rossym)
endif()
endif()

add_subdirectory(rtl)
if (NOT NTDLL_WORK)
add_subdirectory(scrnsave)
add_subdirectory(skiplist)
add_subdirectory(strmiids)
add_subdirectory(smlib)
add_subdirectory(tdilib)
add_subdirectory(tzlib)
add_subdirectory(udmihelp)
endif()
add_subdirectory(uuid)
if (NOT NTDLL_WORK)
add_subdirectory(wdmguid)
endif()

else()

add_subdirectory(3rdparty/zlib)

endif()
