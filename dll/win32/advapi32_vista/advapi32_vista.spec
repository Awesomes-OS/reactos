
@ stdcall RegDeleteTreeA(long str)
@ stdcall RegDeleteTreeW(long wstr)
@ stdcall RegSetKeyValueW(long wstr wstr long ptr long)
@ stdcall RegLoadMUIStringW(ptr wstr wstr long ptr long wstr)
@ stdcall RegLoadMUIStringA(ptr str str long ptr long str)

@ stdcall EventActivityIdControl(long ptr)
@ stdcall EventEnabled(int64 ptr) ntdll.EtwEventEnabled
@ stdcall EventProviderEnabled(int64 long int64)
@ stdcall EventRegister(ptr ptr ptr ptr) ntdll.EtwEventRegister
@ stdcall EventSetInformation(int64 long ptr long) ntdll.EtwEventSetInformation
@ stdcall EventUnregister(int64) ntdll.EtwEventUnregister
@ stdcall EventWrite(int64 ptr long ptr) ntdll.EtwEventWrite
@ stdcall EventWriteTransfer(int64 ptr ptr ptr long ptr)