#include <stddef.h>
#include <stdint.h>
#include "reg.h"
#include "threads.h"
#include <string.h>
#include <stdlib.h>

/* USART TXE Flag
 * This flag is cleared when data is written to USARTx_DR and
 * set when that data is transferred to the TDR
 */
#define USART_FLAG_TXE	((uint16_t) 0x0080)
#define USART_FLAG_RXNE  ((uint16_t) 0x0020)

void usart_init(void)
{
    *(RCC_APB2ENR) |= (uint32_t) (0x00000001 | 0x00000004);
    *(RCC_APB1ENR) |= (uint32_t) (0x00020000);

    /* USART2 Configuration, Rx->PA3, Tx->PA2 */
    *(GPIOA_CRL) = 0x00004B00;
    *(GPIOA_CRH) = 0x44444444;
    *(GPIOA_ODR) = 0x00000000;
    *(GPIOA_BSRR) = 0x00000000;
    *(GPIOA_BRR) = 0x00000000;

    *(USART2_CR1) = 0x0000000C;
    *(USART2_CR2) = 0x00000000;
    *(USART2_CR3) = 0x00000000;
    *(USART2_CR1) |= 0x2000;
}



void print_str(const char *str)
{
    while (*str) {
        while (!(*(USART2_SR) & USART_FLAG_TXE));
        *(USART2_DR) = (*str & 0xFF);
        str++;
    }
}

char getChar()
{
    while(1) {
        if ((*USART2_SR) & USART_FLAG_RXNE)
            return *(USART2_DR) & 0xff;
    }
}

int fib(int number)
{
    if(number==0) return 0;
    int result;
    asm volatile("push {r3, r4, r5}");
    asm (	"mov r0, %[num]"::[num] "r" (number));
    asm (
        "mov r4,#0\n"
        "mov r5,#1\n"
        "fibonacci:\n"
        "add r3,r4,r5\n"
        "mov r4,r5\n"
        "mov r5,r3\n"

        "subs r0,r0,#1\n"
        "cmp r0, #1\n"
        "bgt fibonacci"
    );
    asm ("mov %[fibresult], r3":[fibresult]"=r"(result));
    asm volatile("pop {r3, r4, r5}");
    return result;
}

void reverse(char *s)
{
    for(int i = 0,j = strlen(s)-1; i<j; i++,j--) {
        int c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

void itoa(int n,char *s)
{
    int flag = 1;
    int i = 0;

    if (n<0) {
        n = -n;
        flag = 0;
    }
    while(n != 0) {
        s[i++] = n%10+'0';
        n = n/10;
    }
    if(!flag)
        s[i++] = '-';
    s[i] = '\0';
    reverse(s);
}

void command(char *buffer)
{
    char *command = strtok(buffer," ");
    int num;
    if(!strcmp(command,"fib")) {
        command = strtok(NULL, " ");
        print_str("Fibonacci ");
        print_str(command);
        print_str("-step number is ");
        num=atoi(command);
        itoa(fib(num),command);
        print_str(command);
        print_str("\n");
    } else {}
}

void shell(void *data)
{
    char buffer[100];
    int i;
    while(1) {
        print_str("user@Embeded_System_2015:~$");
        for(i=0; i<100; i++) {
            buffer[i]=getChar();
            print_str(buffer+i);//synchronize
            if(buffer[i]==13) { //if input is ENTER
                buffer[i]='\0';//wipe ENTER
                print_str("\n");
                print_str("user@Embeded_System_2015:~$");
                command(buffer);//send command
                while(i>0)
                    buffer[i]='\0';//clean buffer
            }
        }
    }
}

/* 72MHz */
#define CPU_CLOCK_HZ 72000000

/* 100 ms per tick. */
#define TICK_RATE_HZ 10

int main(void)
{
    const char *str = "Shell";

    usart_init();

    if(thread_create(shell, (void *) str) == -1)
        print_str("shell create failed \r\n");

    /* SysTick configuration */
    *SYSTICK_LOAD = (CPU_CLOCK_HZ / TICK_RATE_HZ) - 1UL;
    *SYSTICK_VAL = 0;
    *SYSTICK_CTRL = 0x07;

    thread_start();

    return 0;
}
