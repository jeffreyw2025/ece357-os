.section .bss
buf:
	.equ BUFSIZE, 4096
	.lcomm BUFFER_DATA, BUFSIZE

.section .text   # Initial declarations
	.globl	_start
_start:
# Read has syscall no 0. Arguments to read are 0, buffer, and size of buffer. read returns # of bytes read
read:
	movq	$0, %rdi
	movq	$BUFFER_DATA, %rsi
	movq	$BUFSIZE, %rdx
	movq	$0x0, %rax
	syscall
# Write has syscall no 1. Arguments to write are 1, buffer, and # of bytes read from reader. write returns # of bytes written
write:
	movq	$1, %rdi
	movq	$BUFFER_DATA, %rsi
	movq	%rax, %rdx	# Return value of read syscall is needed as an argument to write
	movq	$0x1, %rax
	syscall
# Exit has syscall no 60 (dec). The only argument to exit is 0. exit does not return.
exit:
	movq	$0, %rdi
	movq	$0x3C, %rax
	syscall
