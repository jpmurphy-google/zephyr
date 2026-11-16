/*
 * Copyright 2026 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc_miwu.h>

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/init.h>
#include <zephyr/input/input_kbd_matrix.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <soc.h>

#define KBD_NODE DT_NODELABEL(kbd)
#define KBD_ROW_SIZE DT_PROP(KBD_NODE, row_size)

void *npcx_kbd_test_map_regs(uintptr_t addr, size_t len);

/* Map the KBS register block before the driver touches it. */
static int map_kbs_regs(void)
{
	struct kbs_reg *kbs = npcx_kbd_test_map_regs(DT_REG_ADDR(KBD_NODE),
						     DT_REG_SIZE(KBD_NODE));

	__ASSERT(kbs == (void *)DT_REG_ADDR(KBD_NODE),
		 "cannot map KBS registers");
	/* KBSIN reads 1 for released keys: no key is pressed. */
	kbs->KBSIN = 0xff;

	return 0;
}
SYS_INIT(map_kbs_regs, PRE_KERNEL_1, 0);

/* Fake PCC clock controller: the driver only turns its clock on. */
static int fake_clock_on(const struct device *dev, clock_control_subsys_t sys)
{
	return 0;
}

static DEVICE_API(clock_control, fake_clock_api) = {
	.on = fake_clock_on,
};

DEVICE_DT_DEFINE(DT_NODELABEL(pcc), NULL, NULL, NULL, NULL, PRE_KERNEL_1,
		 CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &fake_clock_api);

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	return 0;
}

/* Fake MIWU: records the KSI callbacks and models their interrupts. */
static struct {
	const struct npcx_wui *wui;
	struct miwu_callback *cb;
} ksi_cbs[KBD_ROW_SIZE];
static int ksi_cb_count;
static int ksi_irq_enabled;
static int ksi_irq_enabled_before_kbd_matrix_init;
static int ksi_isr_calls;

void npcx_miwu_init_dev_callback(struct miwu_callback *callback,
				 const struct npcx_wui *dev_wui,
				 miwu_dev_callback_handler_t handler,
				 const struct device *source)
{
	callback->dev_cb.handler = handler;
	callback->dev_cb.params.wui = *dev_wui;
	callback->dev_cb.params.source = source;

	__ASSERT_NO_MSG(ksi_cb_count < ARRAY_SIZE(ksi_cbs));
	ksi_cbs[ksi_cb_count].wui = dev_wui;
	ksi_cbs[ksi_cb_count].cb = callback;
	ksi_cb_count++;
}

int npcx_miwu_manage_callback(struct miwu_callback *cb, bool set)
{
	return 0;
}

int npcx_miwu_interrupt_configure(const struct npcx_wui *wui,
				  enum miwu_int_mode mode,
				  enum miwu_int_trig trig)
{
	return 0;
}

bool npcx_miwu_irq_get_and_clear_pending(const struct npcx_wui *wui)
{
	return false;
}

void npcx_miwu_irq_disable(const struct npcx_wui *wui)
{
}

/*
 * The KSI wake-up input is edge triggered on a low KSI line. If a key line is
 * already low when the driver unmasks it (lid open with a key held, or
 * keyboard noise), the interrupt fires right away and npcx_kbd_ksi_isr()
 * calls k_sem_give() on the keyboard matrix poll_lock.
 */
void npcx_miwu_irq_enable(const struct npcx_wui *wui)
{
	struct miwu_callback *cb = NULL;
	const struct device *dev;
	struct input_kbd_matrix_common_data *data;

	for (int i = 0; i < ksi_cb_count; i++) {
		if (ksi_cbs[i].wui == wui) {
			cb = ksi_cbs[i].cb;
		}
	}
	__ASSERT(cb != NULL, "WUI enabled without a callback");

	dev = cb->dev_cb.params.source;
	data = dev->data;
	ksi_irq_enabled++;

	/*
	 * k_sem_init() in input_kbd_matrix_common_init() sets limit to 1. A
	 * k_sem that was never initialized is all zeros. Giving such a k_sem
	 * from the ISR triggers a HardFault on target hardware, so record the
	 * ordering bug here instead of crashing the test binary.
	 */
	if (data->poll_lock.limit == 0) {
		ksi_irq_enabled_before_kbd_matrix_init++;
		return;
	}

	ksi_isr_calls++;
	cb->dev_cb.handler(dev, &cb->dev_cb.params.wui);
}

/*
 * Regression test for drivers/input/input_npcx_kbd.c: verify
 * npcx_kbd_init() enables KSI wake-up interrupts only after
 * input_kbd_matrix_common_init() initializes poll_lock, so an already-asserted
 * KSI line cannot trigger npcx_kbd_ksi_isr() on an uninitialized semaphore.
 */
ZTEST(npcx_kbd, test_ksi_irq_enabled_after_kbd_matrix_init)
{
	const struct device *dev = DEVICE_DT_GET(KBD_NODE);

	zassert_true(device_is_ready(dev));
	zassert_equal(ksi_irq_enabled, KBD_ROW_SIZE,
		      "expected %d KSI interrupts enabled, got %d",
		      KBD_ROW_SIZE, ksi_irq_enabled);
	zassert_equal(ksi_irq_enabled_before_kbd_matrix_init, 0,
		      "%d KSI interrupt(s) enabled before "
		      "input_kbd_matrix_common_init()",
		      ksi_irq_enabled_before_kbd_matrix_init);
	zassert_equal(ksi_isr_calls, KBD_ROW_SIZE);
}

ZTEST_SUITE(npcx_kbd, NULL, NULL, NULL, NULL, NULL);
