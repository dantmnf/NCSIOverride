%ifidn __OUTPUT_FORMAT__, win64
  bits 64
  %define mangle(x) x

  %macro declare 1
  extern mangle(p%1)
  section .text
      global mangle(Shim%1)
      mangle(Shim%1):
      call shim_prepare2
      jmp [rel mangle(p%1)]
  %endmacro

  section .text
  shim_prepare2:
    cmp dword [rel mangle(shim_prepared)], 0
    jz _continue
    ret
  _continue:
    push r9
    push r8
    push rdx
    push rcx
    call mangle(shim_prepare)
    pop rcx
    pop rdx
    pop r8
    pop r9
    ret

%elifidn __OUTPUT_FORMAT__, win32
  bits 32
  %define mangle(x) _ %+ x

  %macro declare 1
    extern mangle(p%1)
    section .text
      global mangle(Shim%1)
      mangle(Shim%1):
      call shim_prepare
      jmp [mangle(p%1)]
  %endmacro

%endif

section .text
align 8
extern mangle(shim_prepare)
extern mangle(shim_prepared)

%include "shim_declare.inc"
