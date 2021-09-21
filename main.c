// Author: Luke LaCasse
// Date: August 26, 2021
// Title: RISC-V Blinken Lights
// Description: Blink some LEDs using the Freedom Metal Libraries

#include <metal/init.h>
#include <metal/cpu.h>
#include <metal/rtc.h>
#include <metal/interrupt.h>
#include <gpio.h>
#include <stdio.h>

#define RTC_RATE 32768
#define DELAY_SEC(SEC) (SEC * RTC_RATE)


void rtc_isr(int id, void *data){
    struct metal_rtc *rtc = (struct metal_rtc*) data;
    metal_rtc_run(rtc, METAL_RTC_STOP);

    /* Disable RTC interrupt */
    struct metal_interrupt *rtc_intr = metal_rtc_get_interrupt(rtc);
    int rtc_id = metal_rtc_get_interrupt_id(rtc);
    metal_interrupt_disable(rtc_intr, rtc_id);

    /* Clear the pending interrupt by setting the compare to a value
     * larger than the current count */
    metal_rtc_set_compare(rtc, metal_rtc_get_count(rtc) + 1);

    OUTPUT_VAL0 ^= (1 << 22);
    printf("Blink!\n");

    metal_rtc_set_count(rtc, 0);
    metal_rtc_set_compare(rtc, DELAY_SEC(1));
    metal_interrupt_enable(rtc_intr, rtc_id);
    metal_rtc_run(rtc, METAL_RTC_RUN);
}

int main() {

    // Initialize CPU + CPU Interrupt
    struct metal_cpu *cpu = NULL;
    cpu = metal_cpu_get(metal_cpu_get_current_hartid());

    if(cpu == NULL) return 1;  // Error 1 if CPU Init Fail

    struct metal_interrupt *cpu_interrupt = metal_cpu_interrupt_controller(cpu);
    if(cpu_interrupt == NULL) return 1;
    metal_interrupt_init(cpu_interrupt);


    // Configure GPIO22
    INPUT_EN0 &= ~(1 << 22);
    OUTPUT_EN0 |= (1 << 22);
    PUE0 &= ~(1 << 22);
    IOF_EN0 |= (1 << 22);

    // Initialize RTC
    struct metal_rtc *rtc = NULL;
    rtc = metal_rtc_get_device(0);
    if(rtc == NULL) return 1;  // Error 1 if fail to get RTC0
    metal_rtc_run(rtc, METAL_RTC_STOP);

    struct metal_interrupt *rtc_int = NULL;
    rtc_int = metal_rtc_get_interrupt(rtc);
    if(rtc_int == NULL) return 1;
    metal_interrupt_init(rtc_int);

    int rtc_id = metal_rtc_get_interrupt_id(rtc);
    metal_interrupt_register_handler(rtc_int, rtc_id, rtc_isr, rtc);

    metal_rtc_set_rate(rtc, RTC_RATE);
    uint64_t rtc_rate = metal_rtc_get_rate(rtc);
    printf("RTC Count Rate [Hz]: %d\n", (int) rtc_rate);

    metal_rtc_set_count(rtc, 0);
    metal_rtc_set_compare(rtc, DELAY_SEC(1));

    metal_interrupt_enable(rtc_int, rtc_id);
    metal_interrupt_enable(cpu_interrupt, 0);

    metal_rtc_run(rtc, METAL_RTC_RUN);
}
