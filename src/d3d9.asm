
format PE GUI 4.0 DLL at 10000000h
entry start

include 'win32w.inc'

section '.code' code readable executable

	include '..\lib\misc.inc'

proc start hinstDLL,fdwReason,lpvReserved
	cmp	[fdwReason],DLL_PROCESS_ATTACH
	jnz	.fin
	push	[hinstDLL]
	call	[DisableThreadLibraryCalls]
	xor	eax,eax
	push	eax
	push	eax
	push	[hinstDLL]
	push	hook_spotify
	push	eax
	push	eax
	call	[CreateThread]
    .fin:
	mov	eax,1
	ret
endp

proc hook_spotify hmodule
	push	ebx esi
	push	_execname
	call	[GetModuleHandle]
	mov	esi,eax
	push	eax
	call	get_image_size
	xchg	eax,ebx
	push	exception_handler
	push	1
	call	[AddVectoredExceptionHandler]
	mov	[handler],eax
	push	_size_play
	push	_ptrn_play
	push	ebx
	push	esi
	call	find_bytes
	test	eax,eax
	je	.exit
	mov	[spotify_play],eax
	push	_size_volume
	push	_mask_volume
	push	_ptrn_volume
	push	ebx
	push	esi
	call	find_pattern
	test	eax,eax
	je	.exit
	mov	[spotify_loop],eax
	push	eax
	call	apply_page_guard
	push	[spotify_play]
	call	apply_page_guard
    .wait:
	cmp	[is_ad_playing],0
	jnz	.done
	cmp	[shutdown],1
	je	.exit
	push	100
	call	[Sleep]
	jmp	.wait
    .done:
	push	MB_OK
	push	_information
	push	_initialized
	push	0
	call	[MessageBox]
    .fin:
	pop	esi ebx
	ret

    .exit:
	push	[spotify_play]
	call	remove_page_guard
	push	[spotify_loop]
	call	remove_page_guard
	push	MB_OK
	push	_title
	push	_fail
	push	0
	call	[MessageBox]
	push	[handler]
	call	[RemoveVectoredExceptionHandler]
	push	0
	push	[hmodule]
	call	[FreeLibraryAndExitThread]
endp

proc exception_handler ExceptionInfo
  local f32s:fde32s
	push	ebx esi edi
	mov	eax,[ExceptionInfo]
	mov	esi,[eax+EXCEPTION_POINTERS.ExceptionRecord]
	mov	edi,[eax+EXCEPTION_POINTERS.ContextRecord]
	cmp	[esi+EXCEPTION_RECORD.ExceptionCode],STATUS_GUARD_PAGE_VIOLATION
	je	.page_guard
	cmp	[esi+EXCEPTION_RECORD.ExceptionCode],EXCEPTION_SINGLE_STEP
	je	.single_step
    .ignore:
	xor	eax,eax
	jmp	.fin
    .page_guard:
	or	[edi+CONTEXT.EFlags],100h
	mov	eax,[esi+EXCEPTION_RECORD.ExceptionInformation+4]
	cmp	eax,[spotify_play]
	je	.hook_play
	cmp	eax,[spotify_loop]
	je	.hook_loop
	jmp	.done
    .single_step:
	movzx	eax,word [edi+CONTEXT.Dr6]
	and	[edi+CONTEXT.Dr6],0
	test	eax,0fh
	jnz	.ignore
	push	[spotify_loop]
	call	apply_page_guard
	cmp	[is_ad_playing],0
	jnz	.done
	push	[spotify_play]
	call	apply_page_guard
    .done:
	or	eax,-1
    .fin:
	pop	edi esi ebx
	ret

    .hook_play:
	mov	ebx,30
	lea	edx,[f32s]
	mov	ecx,[edi+CONTEXT.Eip]
      .look_for_mov:
	dec	ebx
	je	.err
	dec	ecx
	call	decode
	cmp	[edx+fde32s.opcode],089h
	jnz	.look_for_mov
	cmp	[edx+fde32s.modrm.reg],REG_EAX
	jnz	.look_for_mov
	movzx	eax,[edx+fde32s.modrm.rm]
	mov	ebx,reg_map
	xlatb
	mov	eax,[edi+eax]
	add	eax,[edx+fde32s.disp32]
	mov	[is_ad_playing],eax
	jmp	.done
      .err:
	or	[shutdown],1
	jmp	.done

    .hook_loop:
	cmp	[is_ad_playing],0
	je	.done
	cmp	[volume],0
	je	.get_volume_ptr
	mov	ebx,[volume]
      .is_ad_playing:
	mov	eax,[is_ad_playing]
	cmp	byte [eax],1
	je	.ad_is_playing
	cmp	[old_volume],0
	je	.done
	mov	eax,[old_volume]
	mov	[ebx],eax
	and	[old_volume],0
	jmp	.done
      .ad_is_playing:
	cmp	[old_volume],0
	jnz	.mute_ad
	mov	eax,[ebx]
	mov	[old_volume],eax
      .mute_ad:
	cmp	dword [ebx],0
	je	.done
	and	dword [ebx],0
	jmp	.done
      .get_volume_ptr:
	lea	edx,[f32s]
	mov	ecx,[edi+CONTEXT.Eip]
	call	decode
	movzx	eax,[edx+fde32s.modrm.rm]
	mov	ebx,reg_map
	xlatb
	mov	eax,[edi+eax]
	add	eax,[edx+fde32s.disp32]
	mov	[volume],eax
	jmp	.is_ad_playing
endp

section '.data' data readable writeable

  _execname du 'spotify.exe',0
  _title du 'SpotOffify',0
  _information du 'Information',0
  _initialized du 'SpotOffify initialized',0
  _fail du 'Couldn''t find addresses',0
  _ptrn_play db 033h,0C0h      ; xor eax,eax
	     db 084h,0D2h      ; test dl,dl
	     db 00Fh,095h,0C0h ; setnz al
	     db 083h,0C0h,006h ; add eax,6
  _size_play = $-_ptrn_play
  _ptrn_volume db 0F3h,00Fh,010h,08Eh,000h,000h,000h,000h ; movss xmm1,[esi+xx]
	       db 0F3h,00Fh,010h,086h,000h,000h,000h,000h ; movss xmm0,[esi+xx]
	       db 00Fh,02Eh,0C1h			  ; ucomiss xmm0,xmm1
  _mask_volume db 'xxxx????xxxx????xxx'
  _size_volume = $-_mask_volume
  reg_map db CONTEXT.Eax
	  db CONTEXT.Ecx
	  db CONTEXT.Edx
	  db CONTEXT.Ebx
	  db CONTEXT.Esp
	  db CONTEXT.Ebp
	  db CONTEXT.Esi
	  db CONTEXT.Edi

  shutdown rd 1
  handler rd 1
  spotify_play rd 1
  spotify_loop rd 1
  is_ad_playing rd 1
  volume rd 1
  old_volume rd 1

section '.idata' import data readable

  library kernel32,'KERNEL32.DLL',\
	  user32,'USER32.DLL'

  import kernel32,AddVectoredExceptionHandler,'AddVectoredExceptionHandler',\
		  CloseHandle,'CloseHandle',\
		  CreateThread,'CreateThread',\
		  DisableThreadLibraryCalls,'DisableThreadLibraryCalls',\
		  FreeLibraryAndExitThread,'FreeLibraryAndExitThread',\
		  GetCurrentThreadId,'GetCurrentThreadId',\
		  GetModuleHandle,'GetModuleHandleW',\
		  GetThreadContext,'GetThreadContext',\
		  OpenThread,'OpenThread',\
		  RemoveVectoredExceptionHandler,'RemoveVectoredExceptionHandler',\
		  ResumeThread,'ResumeThread',\
		  SetThreadContext,'SetThreadContext',\
		  Sleep,'Sleep',\
		  SuspendThread,'SuspendThread',\
		  VirtualProtect,'VirtualProtect',\
		  VirtualQuery,'VirtualQuery'

  import user32,MessageBox,'MessageBoxW'

section '.reloc' fixups data discardable
