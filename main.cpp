#include <saml21.h>
#include "driver/Pin.h"
#include "driver/Serial.h"
#include "driver/System.h"
#include "driver/Timer.h"

#include "touch.h"

qtm_acq_node_group_config_t  node_group_config = { .num_sensor_nodes	   = 1,
												   .acq_sensor_type		   = NODE_SELFCAP,
												   .calib_option_select	= CAL_CHRG_5TAU << CAL_CHRG_TIME_POS | CAL_AUTO_TUNE_NONE,
												   .freq_option_select	 = FREQ_SEL_0,
												   .ptc_interrupt_priority = 2 };
qtm_acq_saml21_node_config_t node_config	   = { .node_xmask		  = X_NONE,
											   .node_ymask		  = Y(6),
											   .node_rsel_prsc	= NODE_RSEL_PRSC(RSEL_VAL_0, PRSC_DIV_SEL_4),
											   .node_gain		  = NODE_GAIN(GAIN_4, GAIN_1),
											   .node_oversampling = FILTER_LEVEL_16 };
qtm_acq_node_data_t			 node_data;

uint16_t acq_signal_raw;

qtm_acquisition_control_t qt_acq_set = {
	.qtm_acq_node_group_config = &node_group_config, .qtm_acq_node_config = &node_config, .qtm_acq_node_data = &node_data
};

// Touch Key Configuration

qtm_touch_key_group_config_t key_group_config = { .num_key_sensors				= 1,
												  .sensor_touch_di				= 4,
												  .sensor_max_on_time			= 0,
												  .sensor_anti_touch_di			= 5,
												  .sensor_anti_touch_recal_thr  = RECAL_100,
												  .sensor_touch_drift_rate		= 20,  // multiples of 200ms
												  .sensor_anti_touch_drift_rate = 5,   // multiples of 200ms
												  .sensor_drift_hold_time		= 20,  // multiples of 200ms
												  .sensor_reburst_mode			= REBURST_UNRESOLVED };
qtm_touch_key_config_t		 key_config		  = { .channel_threshold = 20, .channel_hysteresis = HYST_25, .channel_aks_group = NO_AKS_GROUP };
qtm_touch_key_group_data_t   key_group_data;
qtm_touch_key_data_t		 key_data;

qtm_touch_key_control_t key_control = { .qtm_touch_key_group_data   = &key_group_data,
										.qtm_touch_key_group_config = &key_group_config,
										.qtm_touch_key_data			= &key_data,
										.qtm_touch_key_config		= &key_config };

void touch_sensor_config() {
	qtm_ptc_init_acquisition_module(&qt_acq_set);
	qtm_ptc_qtlib_assign_signal_memory(&acq_signal_raw);  // ?! initialise memory for DMA according to atmel start comment
	qtm_enable_sensor_node(&qt_acq_set, 0);				  // enable sensor0
	qtm_calibrate_sensor_node(&qt_acq_set, 0);			  // calibrate sensor0

	qtm_init_sensor_key(&key_control, 0, &node_data);
}

Pin scope(Pin::Port::A, 16, Pin::Function::GPIO_Output | Pin::Function::Low);
Pin led(Pin::Port::A, 27, Pin::Function::GPIO_Output | Pin::Function::Low);

void PTC_Handler() {
	qtm_ptc_clear_interrupt();
	qtm_saml21_ptc_handler_eoc();
}

volatile uint8_t touch_measure_request	 = 0u;
volatile uint8_t touch_postprocess_request = 0u;
volatile uint8_t touch_measurement_done	= 0u;

void TC0_Handler() {
	((TcCount8 *)TC0)->INTFLAG.bit.OVF = 1;  // clear interrupt
	touch_measure_request			   = 1u;
	qtm_update_qtlib_timer(20);
}

void touch_process_measurement_done_callback() {
	// using a flag in case that callback is called from inside the PTC interrupt handler
	touch_postprocess_request = 1;
}

void touch_process() {
	if (touch_measure_request) {
		if (qtm_ptc_start_measurement_seq(&qt_acq_set, touch_process_measurement_done_callback) == TOUCH_SUCCESS) {
			touch_measure_request = 0;
		}
	}

	if (touch_postprocess_request) {
		touch_postprocess_request = 0;
		if (qtm_acquisition_process() == TOUCH_SUCCESS) {
			if (qtm_key_sensors_process(&key_control) != TOUCH_SUCCESS) {
			}
		}
	}

	if ((key_control.qtm_touch_key_group_data->qtm_keys_status & QTM_KEY_REBURST) != 0) {
		touch_measure_request = 1;
	} else {
		touch_measurement_done = 1;
	}
}

int main() {
	/** Configure highest power level (2) to be able to run at 48MHz
	 * and set the Flash wait states accordingly (2) */
	SystemInit();

	/** Configure 16MHz external christal oscillator to feed the FPLL
	 * and setup FPLL @ 48MHz. FPLL is fed undivided into GCLK generator 0 (Main clock)
	 * and FPLL is also fed into GCLK generator 1 to generate a 1MHz clock (for PTC and TC0) */
	SystemCoreClockUpdate();

	// init 20ms timer that triggers touch acquisition
	gclk_enable_clock(TC0_GCLK_ID, GCLK_PCHCTRL_GEN_GCLK1_Val);					   // feed GCLK@1MHz into TC0
	timer_init((TcCount8 *)TC0, timer_prescaler_t::Div256, 78);					   // 20ms interval ((256 * 78) / 1MHz)
	timer_enableInterrupts((TcCount8 *)TC0, TC0_IRQn, 1, timer_interrupt_t::OVF);  // enable overflow interrupt
	timer_enable((TcCount8 *)TC0);												   // enable/start counter

	// PTC clock must be 400kHz ≤ PTC_clk ≤ 4MHz
	gclk_enable_clock(PTC_GCLK_ID, GCLK_PCHCTRL_GEN_GCLK1_Val);  // feed GCLK@1MHz into PTC

	touch_sensor_config();

	while (true) {
		touch_process();
		if (touch_measurement_done) {
			touch_measurement_done = 0;
			if ((key_control.qtm_touch_key_data[0].sensor_state & KEY_TOUCHED_MASK) != 0)
				led.setHigh();
			else
				led.setLow();
		}
	}

	return 0;
}