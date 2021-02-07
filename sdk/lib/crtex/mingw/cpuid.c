#include <crtex_mingw.h>
#include <vcruntime/isa_availability.h>

int __isa_enabled = 0, __isa_available = 0, __favor = 0;

typedef union _CPU_INFO
{
    int AsINT[4];
    struct
    {
        ULONG Eax;
        ULONG Ebx;
        ULONG Ecx;
        ULONG Edx;
    };
} CPU_INFO, *PCPU_INFO;

FORCEINLINE
VOID
KiCpuId(PCPU_INFO CpuInfo, ULONG Function)
{
    __cpuid((int*)CpuInfo->AsINT, Function);
}

static const CHAR CmpIntelID[] = "GenuineIntel";

int __cdecl __isa_available_init(void)
{
    __isa_available = 0;
    __isa_enabled |= 1u << __ISA_AVAILABLE_X86;

    if (!IsProcessorFeaturePresent(PF_XMMI64_INSTRUCTIONS_AVAILABLE))
        return 0;

    CPU_INFO CpuInfo;

    KiCpuId(&CpuInfo, 0);

    ULONG LeavesSupported = CpuInfo.Eax;

    /* Fail if no CPUID Support. */
    if (!LeavesSupported)
        return 0;

    CHAR VendorString[13] = {0};

    /* Copy it and null-terminate it */
    *(ULONG*)&VendorString[0] = CpuInfo.Ebx;
    *(ULONG*)&VendorString[4] = CpuInfo.Edx;
    *(ULONG*)&VendorString[8] = CpuInfo.Ecx;
    VendorString[12] = 0;

    KiCpuId(&CpuInfo, 1);

    if (!strcmp(VendorString, CmpIntelID))
    {
        ULONG AtomMasked = CpuInfo.Eax & 0xFFF3FF0;

        if ((AtomMasked == 0x106C0) || (AtomMasked == 0x20660) || (AtomMasked == 0x20670) || (AtomMasked == 0x30650) || (AtomMasked == 0x30660) || (AtomMasked == 0x30670))
        {
            __favor |= 1u << __FAVOR_ATOM;
        }
    }

    ULONG FeatureInformation = CpuInfo.Ecx;

    if (LeavesSupported >= 7)
    {
        KiCpuId(&CpuInfo, 7);
        if ( CpuInfo.Ebx & (1u << 9) )
            __favor |= 1u << __FAVOR_ENFSTRG;
    }

    __isa_available = __ISA_AVAILABLE_SSE2;
    __isa_enabled |= 1u << __ISA_AVAILABLE_SSE2;

    if (FeatureInformation & (1u << 20))
    {
        __isa_available = __ISA_AVAILABLE_SSE42;
        __isa_enabled |= 1u << __ISA_AVAILABLE_SSE42;

        // OSXSAVE && AVX
        if ((FeatureInformation & (1u << 27)) && (FeatureInformation & (1u << 28)))
        {
            // todo: use XGETBV to get more info:
            // __ISA_AVAILABLE_AVX
            // __ISA_AVAILABLE_AVX2
            // __ISA_AVAILABLE_AVX512
        }
    }

    return 0;
}
