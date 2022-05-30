
.code


; ecx = start_offet, edx = rounds, r8d = color_channels, r9 = raw_pixel_buffer, rsi = transed_pixel_buffer
trans_pixel_avx2_first_Xaris_process proc
	
	mov r10, qword ptr [rsp + 40]
	mov rax, r9
	mov r11, r10
	add rax, rcx
	add r11, rcx	

	single_round:
		mov r9, rax
		sub r9, r8
		vmovdqu ymm0, ymmword ptr [rax]
		vmovdqu ymm1, ymmword ptr [r9]
		add rax, 32
		vpsubb ymm0, ymm0, ymm1
		dec rdx
		vmovdqu ymmword ptr [r11], ymm0

		;add rax, 32
		;dec rdx

		add r11, 32
		cmp rdx, 0
		jne single_round
	vzeroupper
	ret

trans_pixel_avx2_first_Xaris_process endp

; ecx = start_offet, edx = rounds, r8d = color_channels, r9 = raw_pixel_buffer, rsi = transed_pixel_buffer
trans_pixel_avx2_normalround_process proc

	push r12
	push rbx
	mov r10, qword ptr [rsp + 56]
	mov r12, qword ptr [rsp + 64]
	mov rbx, r10
	add rbx, rcx		
	mov r11, r9
	mov rax, r9
	add r11, rcx
	add rax, rcx
	sub r11, r12	
	
	; xmm0 = raw_pixel_buffer[i], xmm1 = raw_pixel_buffer[i - 4], xmm2 = raw_pixel_buffer[i - width], xmm3 = for any calculate if need zero, always zero.
	
	; ymm4 = raw_pixel_buffer[i]-word, ymm5 = raw_pixel_buffer[i - 4]-word, ymm6 = raw [i - width]-word.
	
	; ymm7 = raw_pixel_buffer[i]-lower128bit-word, ymm8 = raw_pixel_buffer[i - 4]-lower128bit-word, ymm9 = raw [i - width]-lower128bit-word.
	
	; ymm5 = vpavgw result register, ymm6 = vpsubw result register, ymm4 = vpsubw result register.
	
	; xmm11 = ymm4 higher 128bit data, xmm12 = ymm4 pack unsigned word to byte result register.
	
	; ymm13 = for ymm4 to clear the higher 8bit in each word value.
	
	mov r10, 000FF00FF00FF00FFh
	vmovq xmm13, r10
	vpbroadcastq ymm13, xmm13
	vpxor ymm3, ymm3, ymm3
	single_round:
		mov r9, rax
		sub r9, r8
		vmovdqu xmm0, xmmword ptr [rax]
		vmovdqu xmm1, xmmword ptr [r9]
		vmovdqu xmm2, xmmword ptr [r11]
		vpunpcklbw ymm5, ymm1, ymm3
		vpunpckhbw ymm8, ymm1, ymm3
		vpunpcklbw ymm6, ymm2, ymm3
		vinserti128 ymm5, ymm5, xmm8, 1
		vpunpckhbw ymm9, ymm2, ymm3		
		vpunpcklbw ymm4, ymm0, ymm3
		vinserti128 ymm6, ymm6, xmm9, 1
		vpunpckhbw ymm7, ymm0, ymm3
		vpaddw ymm5, ymm5, ymm6
		vinserti128 ymm4, ymm4, xmm7, 1
		add rax, 16
		vpavgw ymm10, ymm5, ymm3
		add r11, 16
		vpsubw ymm5, ymm5, ymm10
		dec rdx
		vpsubw ymm4, ymm4, ymm5
		vandpd ymm4, ymm4, ymm13
		vextracti128 xmm11, ymm4, 1
		vpackuswb xmm12, xmm4, xmm11
		vmovdqu xmmword ptr [rbx], xmm12	

		;add rax, 16
		;add r11, 16
		;dec rdx

		add rbx, 16
		cmp rdx, 0
		jne single_round
	vzeroupper
	pop rbx
	pop r12
	ret

trans_pixel_avx2_normalround_process endp

; AVX512 part. You need AVX512(F, VL, DQ, BW)
; Cannot run in Xeon Phi processors (Knights Landing / Knights Mill, etc).
; ecx = start_offet, edx = rounds, r8d = color_channels, r9 = raw_pixel_buffer, rsi = transed_pixel_buffer
; Test passed on 11th Core Engineering Sample chip (Rocket Lake-S). 

trans_pixel_avx512_first_Xaris_process proc
	
	mov r10, qword ptr [rsp + 40]
	mov rax, r9
	mov r11, r10
	add rax, rcx
	add r11, rcx	
	mov r10, 0FFFFFFFFFFFFFFFFh
	kmovq k0, r10
	single_round:
		mov r9, rax
		sub r9, r8
		vmovdqu64 zmm0, zmmword ptr [rax]
		vmovdqu64 zmm1, zmmword ptr [r9]
		add rax, 64
		vpsubb zmm0, zmm0, zmm1
		dec rdx
		vmovdqu64 zmmword ptr [r11], zmm0


		add r11, 64
		cmp rdx, 0
		jne single_round
	vzeroupper
	ret

trans_pixel_avx512_first_Xaris_process endp

; ecx = start_offet, edx = rounds, r8d = color_channels, r9 = raw_pixel_buffer, rsi = transed_pixel_buffer
trans_pixel_avx512_normalround_process proc

	push r12
	push rbx
	mov r10, qword ptr [rsp + 56]
	mov r12, qword ptr [rsp + 64]
	mov rbx, r10
	add rbx, rcx		
	mov r11, r9
	mov rax, r9
	add r11, rcx
	add rax, rcx
	sub r11, r12	
	
	mov r10, 0FFFFFFFFFFFFFFFFh
	kmovq k0, r10
	mov r10, 000FF00FF00FF00FFh
	vmovq xmm13, r10
	vpbroadcastq zmm13, xmm13
	vpxorq zmm3, zmm3, zmm3
	single_round:
		mov r9, rax
		sub r9, r8
		vmovdqu64 ymm0, ymmword ptr [rax]
		vmovdqu64 ymm1, ymmword ptr [r9]
		vmovdqu64 ymm2, ymmword ptr [r11]
		vpunpcklbw zmm5, zmm1, zmm3
		vpunpckhbw zmm8, zmm1, zmm3
		vpunpcklbw zmm6, zmm2, zmm3
		vinserti64x4 zmm5, zmm5, ymm8, 1
		vpunpckhbw zmm9, zmm2, zmm3		
		vpunpcklbw zmm4, zmm0, zmm3
		vinserti64x4 zmm6, zmm6, ymm9, 1
		vpunpckhbw zmm7, zmm0, zmm3
		vpaddw zmm5, zmm5, zmm6
		vinserti64x4 zmm4, zmm4, ymm7, 1
		add rax, 32
		vpavgw zmm10, zmm5, zmm3
		add r11, 32
		vpsubw zmm5, zmm5, zmm10
		dec rdx
		vpsubw zmm4, zmm4, zmm5
		vandpd zmm4, zmm4, zmm13
		vextracti64x4 ymm11, zmm4, 1
		vpackuswb ymm12, ymm4, ymm11
		vmovdqu64 ymmword ptr [rbx], ymm12	

		add rbx, 32
		cmp rdx, 0
		jne single_round
	vzeroupper
	pop rbx
	pop r12
	ret

trans_pixel_avx512_normalround_process endp
end
