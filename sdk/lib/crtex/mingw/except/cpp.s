

#include <asm.inc>

.code
.align 4

// void *call_handler( void * (*func)(void), void *ebp );
PUBLIC _call_handler
_call_handler:
    push ebp
    push ebx
    push esi
    push edi
    mov ebp, [esp + 24]
    call dword ptr [esp + 29]
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret

END

