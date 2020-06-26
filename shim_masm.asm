ifdef rax
  mangle macro name
    exitm <name>
  endm
  extern mangle(shim_prepared):dword
  .code
  shim_prepare2:
    cmp mangle(shim_prepared), 0
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

  PREPARE_NAME textequ <shim_prepare2>
  .code
else
  .386
  .model flat
  mangle macro name
    exitm @catstr(<_>, <name>)
  endm
  PREPARE_NAME textequ <shim_prepare>
  _TEXT$ SEGMENT PARA PUBLIC 'CODE'
endif

declare macro name
  local ptrname, funcname
  ptrname equ mangle(@catstr(<p>, name))
  extern ptrname:ptr
  funcname equ mangle(@catstr(<Shim>, name))
  align 16
  public funcname
  funcname:
  call mangle(%PREPARE_NAME)
  jmp [ptrname]
  
endm

extern mangle(shim_prepared):dword
mangle(shim_prepare) proto


include shim_declare.inc


end
