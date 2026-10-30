@ call_with_irqs(fn): from inside the interrupt handler, run fn with interrupts enabled again, so
@ the step timer can interrupt the drum mixing. Switches to system mode (the main program's stack)
@ and saves what a nested interrupt would overwrite: spsr_irq and lr_irq.
	.section .iwram, "ax", %progbits
	.arm
	.align 2
	.global call_with_irqs
call_with_irqs:
	mrs	r1, spsr
	stmfd	sp!, {r1, lr}		@ on the IRQ stack
	mov	r1, #0x1F		@ system mode, IRQs enabled
	msr	cpsr_c, r1
	stmfd	sp!, {r2, lr}		@ lr_sys belongs to the interrupted program
	mov	lr, pc
	bx	r0
	ldmfd	sp!, {r2, lr}
	mov	r1, #0x92		@ back to IRQ mode, IRQs disabled
	msr	cpsr_c, r1
	ldmfd	sp!, {r1, lr}
	msr	spsr_cxsf, r1
	bx	lr
