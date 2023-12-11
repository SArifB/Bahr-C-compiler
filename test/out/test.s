	.text
	.file	"some_code"
	.globl	add_test                        # -- Begin function add_test
	.p2align	4, 0x90
	.type	add_test,@function
add_test:                               # @add_test
	.cfi_startproc
# %bb.0:                                # %entry
                                        # kill: def $edi killed $edi def $rdi
	leal	20(%rdi), %eax
	retq
.Lfunc_end0:
	.size	add_test, .Lfunc_end0-add_test
	.cfi_endproc
                                        # -- End function
	.section	".note.GNU-stack","",@progbits
