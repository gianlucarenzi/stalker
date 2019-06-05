// based on "ARM Profiling Implementation" from Sourcery G++ Lite for ARM EABI

	.globl __gnu_mcount_nc
	.type __gnu_mcount_nc, %function
	.syntax unified 
	
__gnu_mcount_nc:
	mov    ip, lr
	pop    { lr }
	bx     ip

	.end __gnu_mcount_nc
