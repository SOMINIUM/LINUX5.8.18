# SPDX-License-Identifier: GPL-2.0-only
#
# Makefile for the ia32 kernel emulation subsystem.
#

CFLAGS_ia32_signal.o 			+= -O
obj-$(CONFIG_IA32_EMULATION) := ia32_signal.o

obj-$(CONFIG_IA32_AOUT) += ia32_aout.o

audit-class-$(CONFIG_AUDIT) := audit.o
obj-$(CONFIG_IA32_EMULATION) += $(audit-class-y)
