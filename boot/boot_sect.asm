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
; BIOS int 0x13, AH=0x02 will read from the di*k
;======================================

load_kernel:
	mov si, msg_loading
	call print_string

	mov ah, 0x02
	mov al, 100 ; enough for kernel and future plus doom glue

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

; --------------------------------------
; DETECT EXTENDED MEMORY
;---------------------------------------
detect_memory:
	mov ax, 0xE801
	int 0x15
	jc .mem_fallback ; some crappy bios doesn't support E801 lol :(
	; ax = kb betn 1mb and 16 mb while bx = 64kb blocks above 16mb
	; store ax as a simple extended kb val
	mov [0x0500], ax
	jmp enable_a20

.mem_fallback:
	; we doin the old way AH=0x88 method
	mov ah, 0x88
	int 0x15
	jc .mem_unknown
	mov[0x0500], ax
	jmp enable_a20

.mem_unknown:
	mov word [0x0500], 0 ; kernel assumes as minimum

; ----------------------------------------------------
; ENABLE A20 LINE
;---------------------------------------------------
enable_a20:
	; method 1: bios
	mov ax, 0x2401
	int 0x15 ; lets ignore errors

	; method 2: a20 via port 0x92
	; bit 1 of it: a20 enable_a20
	; bit 0: reset, donot set this or machine will rebooti
	in al, 0x92
	or al, 0x02 ; set bit 1
	and al, 0xFE ; clear bit 0
	out 0x92, al

	; method 33: PS/2 controller
	; wait for input buffer empty , send 0xD1 command
	; then 0xDF
	call .wait_ps2
	mov al, 0xD1
	out 0x64, al
	call .wait_ps2
	mov al, 0xDF
	out 0x60, al
	call .wait_ps2

	mov si, msg_a20
	call print_string
	jmp enter_protected_mode

.wait_ps2:
	in al, 0x64
	test al, 0x02 ; input buffer full?
	jnz .wait_ps2
	ret

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
msg_a20 db "A20 :)", 0x0D, 0x0A, 0

times 510-($-$$) db 0
dw 0xaa55

