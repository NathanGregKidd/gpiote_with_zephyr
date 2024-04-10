/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#include <nrfx_gpiote.h>
#include <helpers/nrfx_gppi.h>
#if defined(DPPI_PRESENT)
#include <nrfx_dppi.h>
#else
#include <nrfx_ppi.h>
#endif

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(nrfx_sample, LOG_LEVEL_INF);

const struct gpio_dt_spec z_data0 = GPIO_DT_SPEC_GET(DT_NODELABEL(data0), gpios);
const struct gpio_dt_spec z_data1 = GPIO_DT_SPEC_GET(DT_NODELABEL(data1), gpios);

//#define INPUT_PIN	DT_GPIO_PIN(DT_NODELABEL(data0), gpios)
//#define OUTPUT_PIN	DT_GPIO_PIN(DT_NODELABEL(data1), gpios)

//I don't know how to get the raw pin number from the device tree so I just
//added 32 to the pin number to get the pins from GPIO 1
#define INPUT_PIN 14+32 //data0 is P1.14
#define OUTPUT_PIN 15+32 //data1 is P1.15

#define GPIOTE_INST	NRF_DT_GPIOTE_INST(DT_NODELABEL(data0), gpios)
#define GPIOTE_NODE	DT_NODELABEL(_CONCAT(gpiote, GPIOTE_INST))


//zephyr RTOS gpio callback
struct gpio_callback z_cb_data;
void z_gpio_cb(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    LOG_INF("Z_GPIO CB!");
}

//NRFX HAL GPIOTE callback (should not get called!)
static void button_handler(nrfx_gpiote_pin_t pin,
			   nrfx_gpiote_trigger_t trigger,
			   void *context)
{
	LOG_INF("GPIO input event callback");
}


//main
int main(void)
{
    uint8_t channel_check = 255; //not needed, just for debugging
	nrfx_err_t err;
	uint8_t in_channel, out_channel;
	uint8_t ppi_channel;
	LOG_INF("nrfx_gpiote sample on %s", CONFIG_BOARD);
    LOG_INF("input pin: %d", INPUT_PIN);
    LOG_INF("output pin: %d", OUTPUT_PIN);
        
    //declare gpiote instance structure
    const nrfx_gpiote_t gpiote = NRFX_GPIOTE_INSTANCE(GPIOTE_INST);
    LOG_INF("gpiote_instance: %d\n", GPIOTE_INST);

    //non-necessary, just debugging
    nrfx_gpiote_channel_get(GPIOTE_INST, INPUT_PIN, &channel_check);
    LOG_INF("data0 gpiote_channel: %d\n", channel_check); 

    //Zephyr API to setup P1.14 as INPUT with interrupt and callback
    // NOTE: this configures the GPIOTE as well (so long as gpio1 in
    // devicetree has sense-edge-mask set correctly, which I didn't 
    // have to touch at all. Just try it, if it doesn't work only then
    // mess with the Devicetree)
    gpio_pin_configure_dt(&z_data0, GPIO_INPUT);
    gpio_pin_interrupt_configure_dt(&z_data0, GPIO_INT_EDGE_BOTH);
    gpio_init_callback(&z_cb_data, z_gpio_cb, BIT(z_data0.pin));
    gpio_add_callback(z_data0.port, &z_cb_data);
    
    //Zephyr API to setup P1.15 as GPIO OUTPUT
    // I do not believe this touches GPIOTE whatsoever, so GPIOTE
    // configuration for the output is still necessary
    gpio_pin_configure_dt(&z_data1, GPIO_OUTPUT_ACTIVE);

    //Debugging, non-necessary. Just checks to see what channel the pins are
    nrfx_gpiote_channel_get(GPIOTE_INST, INPUT_PIN, &channel_check);
    LOG_INF("data0 gpiote_channel: %d\n", channel_check); 
    nrfx_gpiote_channel_get(GPIOTE_INST, OUTPUT_PIN, &channel_check);
    LOG_INF("data1 gpiote_channel: %d\n", channel_check); 

   

    //NRFX interrupt setup. Don't do this because Zephyr already handles this

	/* Connect GPIOTE instance IRQ to irq handler */
    /*
	IRQ_CONNECT(DT_IRQN(GPIOTE_NODE), DT_IRQ(GPIOTE_NODE, priority), nrfx_isr,
		    NRFX_CONCAT(nrfx_gpiote_, GPIOTE_INST, _irq_handler), 0);
    */


    //This function gives the error code for "already initialized" which is no
    //surprise, as the Zephyr GPIO APIs already setup GPIOTE. Don't use this
    //function

	/* Initialize GPIOTE (the interrupt priority passed as the parameter
	 * here is ignored, see nrfx_glue.h).
	 */
    /*
	err = nrfx_gpiote_init(&gpiote, 0);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("nrfx_gpiote_init error: 0x%08X", err);
		return 0;
	}
    */


    //Zephyr already allocates a GPIOTE channel for the input pin. Do not
    //allocate another one as I don't know what kind of side affects this
    //has!

	//err = nrfx_gpiote_channel_alloc(&gpiote, &in_channel);
	//if (err != NRFX_SUCCESS) {
	//	LOG_ERR("Failed to allocate in_channel, error: 0x%08X", err);
	//	return 0;
	//}


    //DO allocate a GPIOTE channel for the output. This is because Zephyr
    //API does not use GPIOTE for outputs?? Not sure
	err = nrfx_gpiote_channel_alloc(&gpiote, &out_channel);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("Failed to allocate out_channel, error: 0x%08X", err);
		return 0;
	}


    //ALL OF THE BELOW is related to input pin GPIOTE setup, which is all
    //automatically handled by gpio_pin_interrupt_configure_dt() etc. 
    //DO NOT do it again!

	/* Initialize input pin to generate event on high to low transition
	 * (falling edge) and call button_handler()
	 */
    //nrfx_gpiote_channel_get(GPIOTE_INST, INPUT_PIN, &in_channel);
    //nrfx_gpiote_channel_get(GPIOTE_INST, OUTPUT_PIN, &out_channel);
    /*
	static const nrf_gpio_pin_pull_t pull_config = NRF_GPIO_PIN_PULLUP;
	nrfx_gpiote_trigger_config_t trigger_config = {
		.trigger = NRFX_GPIOTE_TRIGGER_HITOLO,
		//.trigger = NRFX_GPIOTE_TRIGGER_TOGGLE,
		.p_in_channel = &in_channel,
	};
	static const nrfx_gpiote_handler_config_t handler_config = {
		.handler = button_handler,
	};
	nrfx_gpiote_input_pin_config_t input_config = {
		.p_pull_config = &pull_config,
		.p_trigger_config = &trigger_config,
		.p_handler_config = &handler_config
	};
    */
	//err = nrfx_gpiote_input_configure(&gpiote, INPUT_PIN, &input_config);


    //The below sets up the GPIOTE output pin. The Zephyr API has not done 
    //this yet so you must do this!

	/* Initialize output pin. SET task will turn the LED on,
	 * CLR will turn it off and OUT will toggle it.
	 */
	static const nrfx_gpiote_output_config_t output_config = {
		.drive = NRF_GPIO_PIN_S0S1,
		.input_connect = NRF_GPIO_PIN_INPUT_DISCONNECT,
		.pull = NRF_GPIO_PIN_NOPULL,
	};
	const nrfx_gpiote_task_config_t task_config = {
		.task_ch = out_channel,
		.polarity = NRF_GPIOTE_POLARITY_TOGGLE,
		.init_val = 1,
	};
	err = nrfx_gpiote_output_configure(&gpiote, OUTPUT_PIN,
					   &output_config,
					   &task_config);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("nrfx_gpiote_output_configure error: 0x%08X", err);
		return 0;
	}

    //Once again, don't enable the input here as Zephyr API already did.
	//nrfx_gpiote_trigger_enable(&gpiote, INPUT_PIN, true);
    
    //But DO enable the output because Zephyr hasn't done that
	nrfx_gpiote_out_task_enable(&gpiote, OUTPUT_PIN);

	LOG_INF("nrfx_gpiote initialized");


    //The rest of the program should be the same as the board nrfx example.

	/* Allocate a (D)PPI channel. */
	err = nrfx_gppi_channel_alloc(&ppi_channel);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("nrfx_gppi_channel_alloc error: 0x%08X", err);
		return 0;
	}

	/* Configure endpoints of the channel so that the input pin event is
	 * connected with the output pin OUT task. This means that each time
	 * the button is pressed, the LED pin will be toggled.
	 */
	nrfx_gppi_channel_endpoints_setup(ppi_channel,
		nrfx_gpiote_in_event_address_get(&gpiote, INPUT_PIN),
		nrfx_gpiote_out_task_address_get(&gpiote, OUTPUT_PIN));

	/* Enable the channel. */
	nrfx_gppi_channels_enable(BIT(ppi_channel));

	LOG_INF("(D)PPI configured, leaving main()");
	return 0;
}
