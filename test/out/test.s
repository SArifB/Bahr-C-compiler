	.text
	.file	"test.bh"
	.globl	test                            # -- Begin function test
	.p2align	4, 0x90
	.type	test,@function
test:                                   # @test
	.cfi_startproc
# %bb.0:                                # %entry
	movl	$20, %eax
	retq
.Lfunc_end0:
	.size	test, .Lfunc_end0-test
	.cfi_endproc
                                        # -- End function
	.globl	square                          # -- Begin function square
	.p2align	4, 0x90
	.type	square,@function
square:                                 # @square
	.cfi_startproc
# %bb.0:                                # %entry
	movl	%edi, %eax
	movl	%edi, -4(%rsp)
	imull	%edi, %eax
	retq
.Lfunc_end1:
	.size	square, .Lfunc_end1-square
	.cfi_endproc
                                        # -- End function
	.section	".note.GNU-stack","",@progbits
