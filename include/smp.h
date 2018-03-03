#pragma once

#include <stdint.h>

#include "interrupts.h"

uint8_t smp_cpu_id(void);
void smp_init(void);
void smp_ap_kmain(void);

void ipi_abort(struct isr_ctx *regs);
void ipi_tlb_shootdown(struct isr_ctx *regs);

extern volatile unsigned int smp_nr_cpus_ready;

static inline unsigned int smp_nr_cpus(void)
{
	return __atomic_load_n(&smp_nr_cpus_ready, __ATOMIC_RELAXED);
}
