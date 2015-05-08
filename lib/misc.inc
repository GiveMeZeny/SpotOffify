; --[ memory-related ]

struct MEMORY_BASIC_INFORMATION
  BaseAddress	    rd 1
  AllocationBase    rd 1
  AllocationProtect rd 1
  RegionSize	    rd 1
  State 	    rd 1
  Protect	    rd 1
  Type		    rd 1
ends

proc find_bytes base,size,bytes,length
	push	esi edi
	mov	edi,[base]
	mov	ecx,[size]
	dec	dword [length]
    .loop:
	mov	esi,[bytes]
	mov	al,[esi]
	repnz	scasb
	jnz	.not_found
	mov	edx,ecx
	push	edi
	inc	esi
	mov	ecx,[length]
    .compare:
	repe	cmpsb
	jnz	.no_match
	pop	eax
	dec	eax
	jmp	.fin
    .no_match:
	pop	edi
	mov	ecx,edx
	jmp	.loop
    .not_found:
	xor	eax,eax
    .fin:
	pop	edi esi
	ret
endp

proc find_pattern base,size,pattern,mask,length
	push	ebx esi edi
	mov	edi,[base]
	mov	ecx,[size]
	mov	edx,[mask]
	dec	dword [length]
    .loop:
	mov	esi,[pattern]
	mov	al,[esi]
	repnz	scasb
	jnz	.not_found
	mov	ebx,ecx
	push	edi
	inc	esi
	mov	ecx,[length]
    .compare:
	repe	cmpsb
	jnz	.check_wildcard
	pop	eax
	dec	eax
	jmp	.fin
    .check_wildcard:
	jecxz	.no_match
	mov	eax,[esp]
	sub	eax,edi
	neg	eax
	cmp	byte [edx+eax],'?'
	je	.compare
    .no_match:
	pop	edi
	mov	ecx,ebx
	jmp	.loop
    .not_found:
	xor	eax,eax
    .fin:
	pop	edi esi ebx
	ret
endp

; --[ disasm, hooking & internals ]

EXCEPTION_READ_FAULT		 = 0
EXCEPTION_WRITE_FAULT		 = 1
EXCEPTION_EXECUTE_FAULT 	 = 8

SIZE_OF_80387_REGISTERS 	 = 80

CONTEXT_i386			 = 00010000h
CONTEXT_i486			 = 00010000h

CONTEXT_CONTROL 		 = CONTEXT_i386 or 00000001h
CONTEXT_INTEGER 		 = CONTEXT_i386 or 00000002h
CONTEXT_SEGMENTS		 = CONTEXT_i386 or 00000004h
CONTEXT_FLOATING_POINT		 = CONTEXT_i386 or 00000008h
CONTEXT_DEBUG_REGISTERS 	 = CONTEXT_i386 or 00000010h
CONTEXT_EXTENDED_REGISTERS	 = CONTEXT_i386 or 00000020h

CONTEXT_FULL			 = CONTEXT_CONTROL or CONTEXT_INTEGER or CONTEXT_SEGMENTS
CONTEXT_ALL			 = CONTEXT_FULL or CONTEXT_FLOATING_POINT or CONTEXT_DEBUG_REGISTERS or CONTEXT_EXTENDED_REGISTERS
CONTEXT_XSTATE			 = CONTEXT_i386 or 00000040h

MAXIMUM_SUPPORTED_EXTENSION	 = 512

EXCEPTION_MAXIMUM_PARAMETERS	 = 15

EXCEPTION_UNWINDING		 = 000000002h
EXCEPTION_EXIT_UNWIND		 = 000000004h
EXCEPTION_STACK_INVALID 	 = 000000008h
EXCEPTION_NESTED_CALL		 = 000000010h
EXCEPTION_TARGET_UNWIND 	 = 000000020h
EXCEPTION_COLLIDED_UNWIND	 = 000000040h
EXCEPTION_UNWIND		 = 000000066h
EXCEPTION_EXECUTE_HANDLER	 = 000000001h
EXCEPTION_CONTINUE_SEARCH	 = 000000000h
EXCEPTION_CONTINUE_EXECUTION	 = 0FFFFFFFFh

STATUS_GUARD_PAGE_VIOLATION	 = 080000001h

IMAGE_DOS_SIGNATURE		 = 'MZ'
IMAGE_NT_SIGNATURE		 = 'PE'
IMAGE_NUMBEROF_DIRECTORY_ENTRIES = 16
IMAGE_DIRECTORY_ENTRY_EXPORT	 = 0
IMAGE_DIRECTORY_ENTRY_IMPORT	 = 1
IMAGE_DIRECTORY_ENTRY_IAT	 = 12

struct EXCEPTION_POINTERS
  ExceptionRecord rd 1
  ContextRecord   rd 1
ends

struct EXCEPTION_RECORD
  ExceptionCode        rd 1
  ExceptionFlags       rd 1
  ExceptionRecord      rd 1
  ExceptionAddress     rd 1
  NumberParameters     rd 1
  ExceptionInformation rd EXCEPTION_MAXIMUM_PARAMETERS
ends

struct FLOATING_SAVE_AREA
  ControlWord	rd 1
  StatusWord	rd 1
  TagWord	rd 1
  ErrorOffset	rd 1
  ErrorSelector rd 1
  DataOffset	rd 1
  DataSelector	rd 1
  RegisterArea	rb 1
		rb SIZE_OF_80387_REGISTERS-1
  Cr0NpxState	rd 1
ends

struct M128A
  Low  rq 1
  High rq 1
ends

struct XMM_SAVE_AREA32
  ControlWord	 rw 1
  StatusWord	 rw 1
  TagWord	 rb 1
  Reserved1	 rb 1
  ErrorOpcode	 rw 1
  ErrorOffset	 rd 1
  ErrorSelector  rw 1
  Reserved2	 rw 1
  DataOffset	 rd 1
  DataSelector	 rw 1
  Reserved3	 rw 1
  MxCsr 	 rd 1
  MxCsr_Mask	 rd 1
  FloatRegisters M128A
		 rb sizeof.M128A*(8-1)
  XmmRegisters	 M128A
		 rb sizeof.M128A*(8-1)
  Reserved4	 rb 192
  StackControl	 rd 7
  Cr0NpxState	 rd 1
ends

struct XMM_SAVE_AREA64
  ControlWord	 rw 1
  StatusWord	 rw 1
  TagWord	 rb 1
  Reserved1	 rb 1
  ErrorOpcode	 rw 1
  ErrorOffset	 rd 1
  ErrorSelector  rw 1
  Reserved2	 rw 1
  DataOffset	 rd 1
  DataSelector	 rw 1
  Reserved3	 rw 1
  MxCsr 	 rd 1
  MxCsr_Mask	 rd 1
  FloatRegisters M128A
		 rb sizeof.M128A*(8-1)
  XmmRegisters	 M128A
		 rb sizeof.M128A*(16-1)
  Reserved4	 rb 96
ends

struct XMM_SAVE_AREA XMM_SAVE_AREA32
ends

struct CONTEXT32
  ContextFlags	    rd 1
  Dr0		    rd 1
  Dr1		    rd 1
  Dr2		    rd 1
  Dr3		    rd 1
  Dr6		    rd 1
  Dr7		    rd 1
  FloatSave	    FLOATING_SAVE_AREA
  SegGs 	    rd 1
  SegFs 	    rd 1
  SegEs 	    rd 1
  SegDs 	    rd 1
  Edi		    rd 1
  Esi		    rd 1
  Ebx		    rd 1
  Edx		    rd 1
  Ecx		    rd 1
  Eax		    rd 1
  Ebp		    rd 1
  Eip		    rd 1
  SegCs 	    rd 1
  EFlags	    rd 1
  Esp		    rd 1
  SegSs 	    rd 1
  ExtendedRegisters rb 1
		    rb MAXIMUM_SUPPORTED_EXTENSION-1
ends

struct CONTEXT64
  P1Home	      rq 1
  P2Home	      rq 1
  P3Home	      rq 1
  P4Home	      rq 1
  P5Home	      rq 1
  P6Home	      rq 1
  ContextFlags	      rd 1
  MxCsr 	      rd 1
  SegCs 	      rw 1
  SegDs 	      rw 1
  SegEs 	      rw 1
  SegFs 	      rw 1
  SegGs 	      rw 1
  SegSs 	      rw 1
  EFlags	      rd 1
  Dr0		      rq 1
  Dr1		      rq 1
  Dr2		      rq 1
  Dr3		      rq 1
  Dr4		      rq 1
  Dr5		      rq 1
  Dr6		      rq 1
  Dr7		      rq 1
  Rax		      rq 1
  Rcx		      rq 1
  Rdx		      rq 1
  Rbx		      rq 1
  Rsp		      rq 1
  Rbp		      rq 1
  Rsi		      rq 1
  Rdi		      rq 1
  R8		      rq 1
  R9		      rq 1
  R10		      rq 1
  R11		      rq 1
  R12		      rq 1
  R13		      rq 1
  R14		      rq 1
  R15		      rq 1
  Rip		      rq 1
  union
   FltSave	      XMM_SAVE_AREA64
   struct
   Header	      M128A
		      rb sizeof.M128A*(2-1)
   Legacy	      M128A
		      rb sizeof.M128A*(8-1)
   Xmm0 	      M128A
   Xmm1 	      M128A
   Xmm2 	      M128A
   Xmm3 	      M128A
   Xmm4 	      M128A
   Xmm5 	      M128A
   Xmm6 	      M128A
   Xmm7 	      M128A
   Xmm8 	      M128A
   Xmm9 	      M128A
   Xmm10	      M128A
   Xmm11	      M128A
   Xmm12	      M128A
   Xmm13	      M128A
   Xmm14	      M128A
   Xmm15	      M128A
   ends
  ends
 VectorRegister       M128A
		      rb sizeof.M128A*(26-1)
 VectorControl	      rq 1
 DebugControl	      rq 1
 LastBranchToRip      rq 1
 LastBranchFromRip    rq 1
 LastExceptionToRip   rq 1
 LastExceptionFromRip rq 1
ends

struct CONTEXT CONTEXT32
ends

struct IMAGE_DOS_HEADER
  e_magic    rw 1
  e_cblp     rw 1
  e_cp	     rw 1
  e_crlc     rw 1
  e_cparhdr  rw 1
  e_minalloc rw 1
  e_maxalloc rw 1
  e_ss	     rw 1
  e_sp	     rw 1
  e_csum     rw 1
  e_ip	     rw 1
  e_cs	     rw 1
  e_lfarlc   rw 1
  e_ovno     rw 1
  e_res      rw 4
  e_oemid    rw 1
  e_oeminfo  rw 1
  e_res2     rw 10
  e_lfanew   rd 1
ends

struct IMAGE_FILE_HEADER
  Machine	       rw 1
  NumberOfSections     rw 1
  TimeDateStamp        rd 1
  PointerToSymbolTable rd 1
  NumberOfSymbols      rd 1
  SizeOfOptionalHeader rw 1
  Characteristics      rw 1
ends

struct IMAGE_DATA_DIRECTORY
  VirtualAddress rd 1
  Size		 rd 1
ends

struct IMAGE_OPTIONAL_HEADER32
  Magic 		      rw 1
  MajorLinkerVersion	      rb 1
  MinorLinkerVersion	      rb 1
  SizeOfCode		      rd 1
  SizeOfInitializedData       rd 1
  SizeOfUninitializedData     rd 1
  AddressOfEntryPoint	      rd 1
  BaseOfCode		      rd 1
  BaseOfData		      rd 1
  ImageBase		      rd 1
  SectionAlignment	      rd 1
  FileAlignment 	      rd 1
  MajorOperatingSystemVersion rw 1
  MinorOperatingSystemVersion rw 1
  MajorImageVersion	      rw 1
  MinorImageVersion	      rw 1
  MajorSubsystemVersion       rw 1
  MinorSubsystemVersion       rw 1
  Win32VersionValue	      rd 1
  SizeOfImage		      rd 1
  SizeOfHeaders 	      rd 1
  CheckSum		      rd 1
  Subsystem		      rw 1
  DllCharacteristics	      rw 1
  SizeOfStackReserve	      rd 1
  SizeOfStackCommit	      rd 1
  SizeOfHeapReserve	      rd 1
  SizeOfHeapCommit	      rd 1
  LoaderFlags		      rd 1
  NumberOfRvaAndSizes	      rd 1
  DataDirectory 	      IMAGE_DATA_DIRECTORY
			      rb sizeof.IMAGE_DATA_DIRECTORY*(IMAGE_NUMBEROF_DIRECTORY_ENTRIES-1)
ends

struct IMAGE_OPTIONAL_HEADER64
  Magic 		      rw 1
  MajorLinkerVersion	      rb 1
  MinorLinkerVersion	      rb 1
  SizeOfCode		      rd 1
  SizeOfInitializedData       rd 1
  SizeOfUninitializedData     rd 1
  AddressOfEntryPoint	      rd 1
  BaseOfCode		      rd 1
  ImageBase		      rq 1
  SectionAlignment	      rd 1
  FileAlignment 	      rd 1
  MajorOperatingSystemVersion rw 1
  MinorOperatingSystemVersion rw 1
  MajorImageVersion	      rw 1
  MinorImageVersion	      rw 1
  MajorSubsystemVersion       rw 1
  MinorSubsystemVersion       rw 1
  Win32VersionValue	      rd 1
  SizeOfImage		      rd 1
  SizeOfHeaders 	      rd 1
  CheckSum		      rd 1
  Subsystem		      rw 1
  DllCharacteristics	      rw 1
  SizeOfStackReserve	      rq 1
  SizeOfStackCommit	      rq 1
  SizeOfHeapReserve	      rq 1
  SizeOfHeapCommit	      rq 1
  LoaderFlags		      rd 1
  NumberOfRvaAndSizes	      rd 1
  DataDirectory 	      IMAGE_DATA_DIRECTORY
			      rb sizeof.IMAGE_DATA_DIRECTORY*(IMAGE_NUMBEROF_DIRECTORY_ENTRIES-1)
ends

struct IMAGE_OPTIONAL_HEADER IMAGE_OPTIONAL_HEADER32
ends

struct IMAGE_NT_HEADERS32
  Signature	 rd 1
  FileHeader	 IMAGE_FILE_HEADER
  OptionalHeader IMAGE_OPTIONAL_HEADER32
ends

struct IMAGE_NT_HEADERS64
  Signature	 rd 1
  FileHeader	 IMAGE_FILE_HEADER
  OptionalHeader IMAGE_OPTIONAL_HEADER64
ends

struct IMAGE_NT_HEADERS IMAGE_NT_HEADERS32
ends

struct IMAGE_EXPORT_DIRECTORY
  Characteristics	rd 1
  TimeDateStamp 	rd 1
  MajorVersion		rw 1
  MinorVersion		rw 1
  Name			rd 1
  Base			rd 1
  NumberOfFunctions	rd 1
  NumberOfNames 	rd 1
  AddressOfFunctions	rd 1
  AddressOfNames	rd 1
  AddressOfNameOrdinals rd 1
ends

struct IMAGE_IMPORT_DESCRIPTOR
  union
    Characteristics    rd 1
    OriginalFirstThunk rd 1
  ends
  TimeDateStamp        rd 1
  ForwarderChain       rd 1
  Name		       rd 1
  FirstThunk	       rd 1
ends

struct IMAGE_THUNK_DATA32
  union
    ForwarderString rd 1
    Function	    rd 1
    Ordinal	    rd 1
    AddressOfData   rd 1
  ends
ends

struct IMAGE_THUNK_DATA64
  union
    ForwarderString rq 1
    Function	    rq 1
    Ordinal	    rq 1
    AddressOfData   rq 1
  ends
ends

struct IMAGE_THUNK_DATA IMAGE_THUNK_DATA32
ends

if used decode | used encode
	include 'fde32.inc'
end if

proc get_image_size module
	mov	eax,[module]
	add	eax,[eax+IMAGE_DOS_HEADER.e_lfanew]
	mov	eax,[eax+IMAGE_NT_HEADERS32.OptionalHeader.SizeOfImage]
	ret
endp

proc apply_page_guard address
  local mbi:MEMORY_BASIC_INFORMATION,old:DWORD
	push	ebx
	lea	ebx,[mbi]
	push	sizeof.MEMORY_BASIC_INFORMATION
	push	ebx
	push	[address]
	call	[VirtualQuery]
	mov	edx,[ebx+MEMORY_BASIC_INFORMATION.Protect]
	lea	eax,[old]
	or	edx,PAGE_GUARD
	push	eax
	push	edx
	push	1
	push	[address]
	call	[VirtualProtect]
	pop	ebx
	ret
endp

proc remove_page_guard address
  local mbi:MEMORY_BASIC_INFORMATION,old:DWORD
	push	ebx
	cmp	[address],0
	je	.fin
	lea	ebx,[mbi]
	push	sizeof.MEMORY_BASIC_INFORMATION
	push	ebx
	push	[address]
	call	[VirtualQuery]
	mov	edx,[ebx+MEMORY_BASIC_INFORMATION.Protect]
	lea	eax,[old]
	and	edx,not PAGE_GUARD
	push	eax
	push	edx
	push	1
	push	[address]
	call	[VirtualProtect]
    .fin:
	pop	ebx
	ret
endp

; --[ GUI-related ]

macro setcenter hwnd,parent,mode=0
 { if mode>0
    push SM_CYCAPTION
    call [GetSystemMetrics]
    push eax
   end if
   push hwnd
   push parent
   call calc_mid
   if mode>0
    pop ecx
    if mode=1
     shr ecx,1
    else
     shr ecx,2
    end if
    add edx,ecx
   end if
   push SWP_NOSIZE+SWP_NOZORDER+SWP_NOACTIVATE
   push 0
   push 0
   push edx
   push eax
   push 0
   push hwnd
   call [SetWindowPos] }

proc calc_mid parent,child
  local rect1:RECT,rect2:RECT
	push	ebx esi edi
	lea	esi,[rect1]
	lea	edi,[rect2]
	push	esi
	push	[parent]
	call	[GetWindowRect]
	push	edi
	push	[child]
	call	[GetWindowRect]
	mov	eax,[esi+RECT.right]
	mov	edx,[esi+RECT.bottom]
	sub	eax,[esi+RECT.left]
	sub	edx,[esi+RECT.top]
	shr	eax,1
	shr	edx,1
	mov	ebx,[edi+RECT.right]
	mov	ecx,[edi+RECT.bottom]
	sub	ebx,[edi+RECT.left]
	sub	ecx,[edi+RECT.top]
	shr	ebx,1
	shr	ecx,1
	sub	eax,ebx
	sub	edx,ecx
	add	eax,[esi+RECT.left]
	add	edx,[esi+RECT.top]
	pop	edi esi ebx
	ret
endp

proc CBTMsgBoxHook code,wparam,lparam
	cmp	[code],HCBT_ACTIVATE
	jnz	.fin
	push	[wparam]
	call	[GetParent]
	test	eax,eax
	cmove	eax,[cbtmsgboxhwnd]
	test	eax,eax
	je	.err
	setcenter [wparam],eax
    .err:
	push	[cbtmsgboxhook]
	call	[UnhookWindowsHookEx]
    .fin:
	xor	eax,eax
	ret
endp

proc MMessageBoxW hwnd,text,caption,type,phwnd
	mov	eax,[phwnd]
	mov	[cbtmsgboxhwnd],eax
	call	[GetCurrentThreadId]
	push	eax
	push	0
	push	CBTMsgBoxHook
	push	WH_CBT
	call	[SetWindowsHookExW]
	mov	[cbtmsgboxhook],eax
	push	[type]
	push	[caption]
	push	[text]
	push	[hwnd]
	call	[MessageBoxW]
	ret
endp

macro misc_udata
 {
   if used cbtmsgboxhook | used cbtmsgboxhwnd
    cbtmsgboxhook rd 1
    cbtmsgboxhwnd rd 1
   end if
 }
