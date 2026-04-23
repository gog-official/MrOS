; ====================================
; boot/boot.asm
; loads this file at 0x7c00
; cool, now lets print message, load kernels, and jump
; ====================================

[org 0x7c00]
[bits 16] ; we start in real mode (16-bit)
; ------------------------------------
; BOOT* ENTRY POINT
; ------------------------------------
start:
	; Segments, BIOS may do dirty
	xor ax, ax
	mov ds, ax
	mov es, ax
	mov ss, ax ; stack segment 0 too

	; Stack away from 0x7c00
	mov bp, 0x8000
	mov sp, bp

	; wipe the direction flag
	cld
	;vd mode
	mov ah, 0x00
	mov al, 0x03
	int 0x10
	cld
	
	; what can I say except you are welcome
	mov si, msg_boot
	call print_string



;=====================================
; LOAD KERNEL FROM DISK
; we will load kernel to address 0x1000:0x0000, yes
; BIOS int 0x13, AH=0x02 will read from the di*k(disk)
;======================================

load_kernel:
	mov si, msg_loading
	call print_string

	mov ah, 0x02
	mov al, 10

	mov ch, 0
	mov cl, 2
	mov dh, 0
	; dl = drive number, BIOS already set this up for us(not dirty btw), dont touch this boy

	mov bx, 0x1000
	mov es, bx
	xor bx, bx ; reset

	int 0x13
	jc disk_error ; silent errors sucks

	mov si, msg_ok
	call print_string

; ---------------------------------------
; SWITCH TO PROTECTED MODE(32 btw)
; ---------------------------------------
enter_protected_mode:
	cli ; disabled interrupts

	lgdt [gdt_descriptor] ; load my GDT into GDTR register

	mov eax, cr0 ; set PE bit in CR0
	or eax, 0x1 ; bit 0 = protection ;)
	mov cr0, eax

	; far jump
	jmp 0x08:init_pm ; 0x08 = first descriptor after null

; -------------------------
; ERROR HANDLER FOR DI*K(DISK)
disk_error:
	mov si, msg_error
	call print_string
	jmp $ ;hang forever

; ----------------------------------
; PRINT FUNCTION
;------------------------------------

print_string:
	mov ah, 0x0e
.loop:
	lodsb
	cmp al, 0
	je .done
	int 0x10
	jmp .loop
.done: 
	ret

; ------------------------------------------
; GDT, flat model used, all 4GB
; -------------------------------------------
gdt_start:
	dd 0x0
	dd 0x0
;-----------------------------
; CSD(CODE SEGMENT DESCRIPTOR, 0x08)
;-----------------------------

gdt_code:
	dw 0xffff
	dw 0x0000
	db 0x00
	db 0x9a
	db 0xcf
	db 0x00
; ------------------------
; CSD 0x10
;--------------------------
gdt_data:
	dw 0xffff
	dw 0x0000
	db 0x00
	db 0x92 ; acess byte
	db 0xcf
	db 0x00

gdt_end:
gdt_descriptor:
	dw gdt_end - gdt_start - 1 ; GDT  - 1
	dd gdt_start ; addr of gdt

; -----------------------------
; PROTECTED MODE CODE (32-bit)
;------------------------------
[bits 32]
init_pm:
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	; new stack
	mov esp, 0x90000

	;jmp to kernel
	jmp 0x08:0x10000 ; CS: offset

; ----------------------
; STRs
; -----------------------

[bits 16]
msg_boot db "MrOs is bootyng, ready for pump? ....", 0x0D, 0x0A, 0
msg_loading db "Loading krnl from disk", 0x0D, 0x0A, 0
msg_ok db "kernel loaded OK!", 0x0D, 0x0A, 0
msg_error db " DISK READ ERROR! HAlting, LIGHTWEIGHT!!!", 0x0D, 0x0A, 0

times 510-($-$$) db 0
dw 0xaa55

