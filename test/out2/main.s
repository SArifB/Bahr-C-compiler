	.text
	.file	"some_code"
	.globl	main                            # -- Begin function main
	.p2align	4, 0x90
	.type	main,@function
main:                                   # @main
	.cfi_startproc
# %bb.0:                                # %entry
	subq	$24, %rsp
	.cfi_def_cfa_offset 32
	movl	%edi, 4(%rsp)
	movq	%rsi, 16(%rsp)
	movq	(%rsi), %rdi
	callq	puts@PLT
	leaq	.L.str(%rip), %rdi
	movq	%rdi, 8(%rsp)
	callq	puts@PLT
	leaq	.L.str.1(%rip), %rdi
	callq	puts@PLT
	xorl	%eax, %eax
	addq	$24, %rsp
	.cfi_def_cfa_offset 8
	retq
.Lfunc_end0:
	.size	main, .Lfunc_end0-main
	.cfi_endproc
                                        # -- End function
	.type	.L.str,@object                  # @.str
	.data
.L.str:
	.asciz	"hello\nworld"
	.size	.L.str, 12

	.type	.L.str.1,@object                # @.str.1
.L.str.1:
	.asciz	"some text"
	.size	.L.str.1, 10

	.section	".note.GNU-stack","",@progbits
