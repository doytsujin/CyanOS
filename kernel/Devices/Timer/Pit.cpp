#include "Pit.h"
#include "Tasking/Scheduler.h"

unsigned PIT::ticks;
void PIT::setup()
{
	ticks = 0;
	PIC::enable_irq(PIC_PIT);
	ISR::register_isr_handler(pit_handler, PIC_PIT + PIC1_IDT_OFFSET);
	out8(I86_PIT_REG_COMMAND, I86_PIT_OCW_COUNTER_0 | I86_PIT_OCW_RL_DATA | I86_PIT_OCW_MODE_SQUAREWAVEGEN);
	out8(I86_PIT_REG_COUNTER0, 0x54); // Lower Half
	out8(I86_PIT_REG_COUNTER0, 0x02); // Higher Half

	// out8(I86_PIT_REG_COUNTER0, 0xff); // Lower Half
	// out8(I86_PIT_REG_COUNTER0, 0xff); // Higher Half
}

void PIT::pit_handler(ISRContextFrame* frame)
{
	Scheduler::schedule(frame, ScheduleType::TIMED);
	ticks++;
	PIC::acknowledge_pic(PIC_PIT);
}