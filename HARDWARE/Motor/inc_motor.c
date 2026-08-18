#include "inc_motor.h"
#include "delay.h"

static void gpio_config(void);
static void adc_config(void);

__IO uint16_t adc1_ordinary_valuetab[2] = {0};

/**
 * @brief  gpio configuration.
 * @param  none
 * @retval none
 */
static void gpio_config(void)
{
	gpio_init_type gpio_initstructure;
	crm_periph_clock_enable(CRM_GPIOC_PERIPH_CLOCK, TRUE);

	gpio_default_para_init(&gpio_initstructure);

	/* config adc pin as analog input mode */
	gpio_initstructure.gpio_mode = GPIO_MODE_ANALOG;
	gpio_initstructure.gpio_pins = GPIO_PINS_2 | GPIO_PINS_1;
	gpio_init(GPIOC, &gpio_initstructure);
}

/**
 * @brief  adc configuration.
 * @param  none
 * @retval none
 */
static void adc_config(void)
{
	dma_init_type dma_init_struct;
	crm_periph_clock_enable(CRM_DMA1_PERIPH_CLOCK, TRUE);

	dma_reset(DMA1_CHANNEL1);
	dma_default_para_init(&dma_init_struct);
	dma_init_struct.buffer_size = 2;
	dma_init_struct.direction = DMA_DIR_PERIPHERAL_TO_MEMORY;
	dma_init_struct.memory_base_addr = (uint32_t)adc1_ordinary_valuetab;
	dma_init_struct.memory_data_width = DMA_MEMORY_DATA_WIDTH_HALFWORD;
	dma_init_struct.memory_inc_enable = TRUE;
	dma_init_struct.peripheral_base_addr = (uint32_t)&(ADC1->odt);
	dma_init_struct.peripheral_data_width = DMA_PERIPHERAL_DATA_WIDTH_HALFWORD;
	dma_init_struct.peripheral_inc_enable = FALSE;
	dma_init_struct.priority = DMA_PRIORITY_HIGH;
	dma_init_struct.loop_mode_enable = TRUE;
	dma_init(DMA1_CHANNEL1, &dma_init_struct);

	dmamux_enable(DMA1, TRUE);
	dmamux_init(DMA1MUX_CHANNEL1, DMAMUX_DMAREQ_ID_ADC1);

	/* enable dma transfer complete interrupt */
	dma_interrupt_enable(DMA1_CHANNEL1, DMA_FDT_INT, TRUE);
	dma_channel_enable(DMA1_CHANNEL1, TRUE);

	adc_common_config_type adc_common_struct;
	adc_base_config_type adc_base_struct;
	crm_periph_clock_enable(CRM_ADC1_PERIPH_CLOCK, TRUE);
	nvic_irq_enable(ADC1_2_3_IRQn, 0, 0);

	adc_common_default_para_init(&adc_common_struct);

	/* config combine mode */
	adc_common_struct.combine_mode = ADC_INDEPENDENT_MODE;

	/* config division,adcclk is division by hclk */
	adc_common_struct.div = ADC_HCLK_DIV_4;

	/* config common dma mode,it's not useful in independent mode */
	adc_common_struct.common_dma_mode = ADC_COMMON_DMAMODE_DISABLE;

	/* config common dma request repeat */
	adc_common_struct.common_dma_request_repeat_state = FALSE;

	/* config adjacent adc sampling interval,it's useful for ordinary shifting mode */
	adc_common_struct.sampling_interval = ADC_SAMPLING_INTERVAL_5CYCLES;

	/* config inner temperature sensor and vintrv */
	adc_common_struct.tempervintrv_state = FALSE;

	/* config voltage battery */
	adc_common_struct.vbat_state = FALSE;
	adc_common_config(&adc_common_struct);

	adc_base_default_para_init(&adc_base_struct);

	adc_base_struct.sequence_mode = TRUE;
	adc_base_struct.repeat_mode = TRUE;
	adc_base_struct.data_align = ADC_RIGHT_ALIGNMENT;
	adc_base_struct.ordinary_channel_length = 2;
	adc_base_config(ADC1, &adc_base_struct);
	adc_resolution_set(ADC1, ADC_RESOLUTION_12B);

	/* config ordinary channel */
	adc_ordinary_channel_set(ADC1, ADC_CHANNEL_11, 1, ADC_SAMPLETIME_47_5);
	adc_ordinary_channel_set(ADC1, ADC_CHANNEL_12, 2, ADC_SAMPLETIME_47_5);

	/* config ordinary trigger source and trigger edge */
	adc_ordinary_conversion_trigger_set(ADC1, ADC_ORDINARY_TRIG_TMR1CH1, ADC_ORDINARY_TRIG_EDGE_NONE);

	/* config dma mode,it's not useful when common dma mode is use */
	adc_dma_mode_enable(ADC1, TRUE);

	/* config dma request repeat,it's not useful when common dma mode is use */
	adc_dma_request_repeat_enable(ADC1, TRUE);

	/* enable adc overflow interrupt */
	adc_interrupt_enable(ADC1, ADC_OCCO_INT, TRUE);

	/* adc enable */
	adc_enable(ADC1, TRUE);
	while (adc_flag_get(ADC1, ADC_RDY_FLAG) == RESET)
		;

//	/* adc calibration */
//	adc_calibration_init(ADC1);
//	while (adc_calibration_init_status_get(ADC1))
//		;
//	adc_calibration_start(ADC1);
//	while (adc_calibration_status_get(ADC1))
//		;

	adc_ordinary_software_trigger_enable(ADC1, TRUE);
}

void TMR_pre_init(tmr_type *tmr_x)
{
	/* tmr3 time base configuration */
	tmr_base_init(tmr_x, 999, 287);
	tmr_cnt_dir_set(tmr_x, TMR_COUNT_UP);
	tmr_clock_source_div_set(tmr_x, TMR_CLOCK_DIV1);

	tmr_output_config_type tmr_oc_init_structure = {0};
	tmr_output_default_para_init(&tmr_oc_init_structure);
	tmr_oc_init_structure.oc_mode = TMR_OUTPUT_CONTROL_PWM_MODE_A;
	tmr_oc_init_structure.oc_idle_state = FALSE;
	tmr_oc_init_structure.oc_polarity = TMR_OUTPUT_ACTIVE_HIGH;
	tmr_oc_init_structure.oc_output_state = TRUE;
	tmr_output_channel_config(tmr_x, TMR_SELECT_CHANNEL_1, &tmr_oc_init_structure);
	tmr_channel_value_set(tmr_x, TMR_SELECT_CHANNEL_1, 0);
	tmr_output_channel_buffer_enable(tmr_x, TMR_SELECT_CHANNEL_1, TRUE);

	tmr_output_channel_config(tmr_x, TMR_SELECT_CHANNEL_2, &tmr_oc_init_structure);
	tmr_channel_value_set(tmr_x, TMR_SELECT_CHANNEL_2, 0);
	tmr_output_channel_buffer_enable(tmr_x, TMR_SELECT_CHANNEL_2, TRUE);

	tmr_period_buffer_enable(tmr_x, TRUE);

	/* tmr enable counter */
	tmr_counter_enable(tmr_x, TRUE);
}

/*
tmr pwm freq set: 10KHz

*/

static void tmr_both_init(void)
{
	CRM->apb1en_bit.tmr3en = TRUE;
	CRM->apb1en_bit.tmr5en = TRUE;
	CRM->ahben1_bit.gpioaen = TRUE;
	CRM->ahben1_bit.gpioben = TRUE;

	gpio_init_type gpio_init_struct;

	gpio_default_para_init(&gpio_init_struct);

	gpio_init_struct.gpio_pins = GPIO_PINS_4 | GPIO_PINS_5;
	gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
	gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
	gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
	gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
	gpio_init(GPIOB, &gpio_init_struct);

	gpio_init_struct.gpio_pins = GPIO_PINS_0 | GPIO_PINS_1;
	gpio_init(GPIOA, &gpio_init_struct);

	gpio_pin_mux_config(GPIOA, GPIO_PINS_SOURCE0, GPIO_MUX_2);
	gpio_pin_mux_config(GPIOA, GPIO_PINS_SOURCE1, GPIO_MUX_2);
	gpio_pin_mux_config(GPIOB, GPIO_PINS_SOURCE4, GPIO_MUX_2);
	gpio_pin_mux_config(GPIOB, GPIO_PINS_SOURCE5, GPIO_MUX_2);

	TMR_pre_init(TMR3);
	TMR_pre_init(TMR5);
}

#define absP(n) ((n>0)?n:0)
#define absN(n) ((n<0)?-n:0)
//电机设置
void Motor_Ctl_Set(int16_t motor1, int16_t motor2)
{
	tmr_channel_value_set(TMR3, TMR_SELECT_CHANNEL_1, absP(motor1));
  tmr_channel_value_set(TMR3, TMR_SELECT_CHANNEL_2, absN(motor1));
	tmr_channel_value_set(TMR5, TMR_SELECT_CHANNEL_1, absP(motor2));
  tmr_channel_value_set(TMR5, TMR_SELECT_CHANNEL_2, absN(motor2));
}

//判断是否过流
uint8_t overCurrent(void)
{
	uint8_t res=0;
  // 计算两个电机的电压（mv)和电流(A)
  float voltage1 = adc1_ordinary_valuetab[0] / 4096.0f * 3.3f * 1000.0f; // mV
  float voltage2 = adc1_ordinary_valuetab[1] / 4096.0f * 3.3f * 1000.0f; // mV
  float current1 = (voltage1 - 16.0f) / 48.0f; // A
  float current2 = (voltage2 - 16.0f) / 48.0f; // A
  float max_voltage = 65.0f; // mV
  float max_current = (max_voltage - 16.0f) / 48.0f; //A  
  //        //if (voltage1 >= max_voltage || voltage2 >= max_voltage)
  if (current1 >= max_current)
  { 
  res=1;
  }
	if(current2 >= max_current)
	{
		res|=2; 
	}
    return res;

}

void Motor_Ctl_Handle()
{
}

void motor_init(void)
{
	gpio_config();
//	adc_config();
	tmr_both_init();
	Motor_Ctl_Set(0,0);
}