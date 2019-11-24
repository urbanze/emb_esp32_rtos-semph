#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <esp_system.h>

SemaphoreHandle_t SMF;//Objeto do semaforo

void isr(void*z)
{
	BaseType_t aux = false;//Variavel de controle para a Troca de Contexto
	xSemaphoreGiveFromISR(SMF, &aux);//Libera o semaforo

	if (aux)
	{
		portYIELD_FROM_ISR();//Se houver tarefas esperando pelo semaforo, deve ser forcado a Troca de Contexto (afim de minimizar latencia)
	}

}


void t1(void*z)
{
	while(1)
	{
		if (xSemaphoreTake(SMF, pdMS_TO_TICKS(200)) == true)//Tenta obter o semaforo durante 200ms (Timeout). Caso o semaforo nao fique disponivel em 200ms, retornara FALSE
		{
			//Se obteu o semaforo entre os 200ms de espera, fara o toggle do pino 23

			for (uint8_t i = 0; i < 10; i++)
			{
				gpio_set_level(GPIO_NUM_23, 1);
				ets_delay_us(150);
				gpio_set_level(GPIO_NUM_23, 0);
				ets_delay_us(150);
			}
		}
		else
		{
			//Se nao obter o semaforo entre os 200ms de espera, fara o toggle do pino 22

			for (uint8_t i = 0; i < 10; i++)
			{
				gpio_set_level(GPIO_NUM_22, 1);
				ets_delay_us(150);
				gpio_set_level(GPIO_NUM_22, 0);
				ets_delay_us(150);
			}
		}

		vTaskDelay(pdMS_TO_TICKS(100));
	}
}

void app_main()
{

	SMF = xSemaphoreCreateBinary();//Cria o semaforo binario e atribui ao objeto que criamos


	//Configura o GPIO22 e GPIO32 como OUTPUT em LOW
	gpio_pad_select_gpio(GPIO_NUM_22);
	gpio_set_direction(GPIO_NUM_22, GPIO_MODE_OUTPUT);
	gpio_set_level(GPIO_NUM_22, 0);
	gpio_pad_select_gpio(GPIO_NUM_23);
	gpio_set_direction(GPIO_NUM_23, GPIO_MODE_OUTPUT);
	gpio_set_level(GPIO_NUM_23, 0);


	//Configura o GPIO4 como INPUT e PULL UP
	gpio_pad_select_gpio(GPIO_NUM_4);
	gpio_set_direction(GPIO_NUM_4, GPIO_MODE_INPUT);
	gpio_pad_pullup(GPIO_NUM_4);

	//Configura a interrupcao em rampas de decida para o GPIO4
	gpio_install_isr_service(ESP_INTR_FLAG_LEVEL1);
	gpio_intr_enable(GPIO_NUM_4);
	gpio_set_intr_type(GPIO_NUM_4, GPIO_INTR_NEGEDGE);
	gpio_isr_handler_add(GPIO_NUM_4, isr, NULL);

	xTaskCreatePinnedToCore(t1, "t1", 4096, NULL, 1, NULL, 0);//Cria a tarefa que analisa o semaforo
}
