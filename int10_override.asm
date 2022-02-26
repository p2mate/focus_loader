	.MODEL	SMALL
	.CODE

	public int10_override_, setup_int10_

int10_override_	proc	far

	cmp 	ax, 4f01h
	je	check_mode01
	cmp	ax, 4f02h
	jne	chain_to_prev

	cmp	bx, 4136h
	jne	chain_to_prev

	push	cx
	push	bx
	and	bx, 4000h
	or	bx, [mode]
	jmp	chain_to_prev_and_restore

check_mode01:
	cmp	cx, 136h
	jne	chain_to_prev

	push	cx
	push	bx
	and	cx, 4000h
	or	cx, [mode]

chain_to_prev_and_restore:
	pushf
	call	[cs:chain_int10]
	pop	bx
	pop	cx
	iret

chain_to_prev:	
	jmp	[cs:chain_int10]

int10_override_	endp

setup_int10_ 	proc	near

	mov	[mode], ax
	mov	dx, cs
	mov	ax, offset chain_int10
	ret

setup_int10_	 endp
	
chain_int10	dd	-1
mode	dw	0

	end
