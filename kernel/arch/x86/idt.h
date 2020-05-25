#pragma once

#include "types.h"

#define NUMBER_OF_IDT_ENTRIES 255

enum IDT_ENTRY_FLAGS {
	PRESENT = 0x80,
	RING3 = 0x60,
	RING0 = 0x00,
	TRAP_GATE = 0x7,
	INT_GATE = 0x6,
	TASK_GATE = 0x5,
	GATE_32 = 0x8,
	GATE_16 = 0
};

#pragma pack(1)
struct IDT {
	uint16_t limit;
	uint32_t base;
};
struct IDTEntry {
	uint16_t offset0_15;
	uint16_t segment;
	uint8_t zero;
	uint8_t type;
	uint16_t offset16_31;
};
#pragma pack()

void setup_idt();
static void fill_idt(IDT* idt, uint32_t base, uint16_t limit);
static void fill_idt_entry(IDTEntry* idt_entry, uint32_t address, uint16_t segment, uint8_t type);
static void load_idt(IDT* idt);

extern IDT idt;
extern IDTEntry idt_entries[];
