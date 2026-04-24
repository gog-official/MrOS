; no bluff, fairly simple to understand
global kernel_entry

[bits 32]

extern kmain
extern _bss_start
extern _bss_end

kernel_entry:
	mov edi, _bss_start
	mov ecx, _bss_end
	sub ecx, edi
	xor eax, eax
	rep stosb
	call kmain
	; if kmain returns:
.hang:
	hlt
	jmp .hang
