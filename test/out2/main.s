	.text
	.file	"some_code"
	.globl	main                            # -- Begin function main
	.p2align	4, 0x90
	.type	main,@function
main:                                   # @main
	.cfi_startproc
# %bb.0:                                # %entry
	pushq	%rax
	.cfi_def_cfa_offset 16
	leaq	.L.str(%rip), %rdi
	movq	%rdi, (%rsp)
	callq	puts@PLT
	xorl	%eax, %eax
	popq	%rcx
	.cfi_def_cfa_offset 8
	retq
.Lfunc_end0:
	.size	main, .Lfunc_end0-main
	.cfi_endproc
                                        # -- End function
	.type	.L.str,@object                  # @.str
	.data
.L.str:
	.asciz	"hello world"
	.size	.L.str, 12

	.section	".note.GNU-stack","",@progbits