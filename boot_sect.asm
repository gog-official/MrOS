[org 0x7c00]

start:
	; welcome, don't rate comments much, i m trying
	xor ax, ax
	mov ds, ax
	mov es, ax

	; wipe the direction flag
	cld

	mov ah, 0x0e ; BIOS teletype

	mov si, message

print:
	lodsb ; load byte from [si] into al
	cmp al, 0
	je done
	int 0x10 ; to print smth
	jmp print

done: 
	jmp $

message db "MrOS booting, get ready for a pump...."
times 510-($-$$) db 0
dw 0xaa55
