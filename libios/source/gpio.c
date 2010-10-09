#include "gctypes.h"
#include "gpio.h"
#include "syscalls.h"

static u32 toggle_mask = 0xFFFFFFFF;

static u32 get_starlet_gpio(u32 flag)
{
	// give starlet control of this gpio
	u32 owner = *(u32*)(GPIO_BASE+GPIO_IS_PPC);
	os_poke_gpios(GPIO_IS_PPC, owner&~flag);

	return owner;
}

static u32 set_gpio_dir(u32 flag, u32 out)
{
	u32 data;

	data = *(u32*)(GPIO_BASE+GPIO_DIR);
	if (out)
		os_poke_gpios(GPIO_DIR, data|flag);
	else
		os_poke_gpios(GPIO_DIR, data&~flag);
	return data;
}

void gpio_set_on(u32 flag)
{
	u32 owner;
	u32 data;

	owner = get_starlet_gpio(flag);

	// set the direction to out
	set_gpio_dir(flag, 1);

	// enable the output
	data = *(u32*)(GPIO_BASE+GPIO_OUT);
	os_poke_gpios(GPIO_OUT, data|flag);

	// restore
	os_poke_gpios(GPIO_IS_PPC, owner);
}

void gpio_set_off(u32 flag)
{
	u32 owner;
	u32 data;

	owner = get_starlet_gpio(flag);

	set_gpio_dir(flag, 1);

	data = *(u32*)(GPIO_BASE+GPIO_OUT);
	os_poke_gpios(GPIO_OUT, data&~flag);

	os_poke_gpios(GPIO_IS_PPC, owner);
}

void gpio_set_toggle(u32 flag)
{
	u32 owner;
	u32 data;

	if (!(flag & toggle_mask))
		return;

	owner = get_starlet_gpio(flag);

	set_gpio_dir(flag, 1);

	data = *(u32*)(GPIO_BASE+GPIO_OUT);
	os_poke_gpios(GPIO_OUT, data^flag);

	os_poke_gpios(GPIO_IS_PPC, owner);
}

bool gpio_get(u32 flag)
{
	u32 owner;
	u32 data;

	owner = get_starlet_gpio(flag);

	// set the direction to in
	set_gpio_dir(flag, 0);

	// get status
	data = *(u32*)(GPIO_BASE+GPIO_IN);

	// restore
	os_poke_gpios(GPIO_IS_PPC, owner);

	return (data&flag);
}

void gpio_enable_toggle(u32 flag)
{
	toggle_mask |= flag;
}

void gpio_disable_toggle(u32 flag)
{
	toggle_mask &= ~flag;
}

/*
slot functions look like this:
	gpio_set_on(GPIO_OSLOT);
	gpio_set_off(GPIO_OSLOT);
for lulz:
        while (gpio_get(GPIO_IDISC)) gpio_set_on(GPIO_OEJECT);
*/
