/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under Ultimate Liberty license
 * SLA0044, the "License"; You may not use this file except in compliance with
 * the License. You may obtain a copy of the License at:
 *                             www.st.com/SLA0044
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "lwip.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ping_helper.h"
#include <string.h> /* memset() */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* overwrite putchar to write to USART1 */
int __io_putchar(int ch) {
	uint8_t asByte = (uint8_t) ch;
	HAL_UART_Transmit(&huart1, &asByte, 1, 1000UL);
	return ch;
}

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void check_debug_info(void* arg) {
	/* check whether the PHY is correctly connected (at least REFCLK, MDIO, MDC)
	 * by receiving the PHY Identifier 1 and 2.
	 * for LAN8720:
	 * IDENT1: 0x0007
	 * IDENT2: 0xc0f<revision number, for me 1>
	 */
	printf("Ethernet own IP: %d.%d.%d.%d.\n", (int)ip4_addr1(&gnetif.ip_addr),
			(int)ip4_addr2(&gnetif.ip_addr), (int)ip4_addr3(&gnetif.ip_addr),
			(int)ip4_addr4(&gnetif.ip_addr));
	uint32_t phy_ident_1 = 0;
	uint32_t phy_ident_2 = 0;
	//try alternating phy address on every cycle (0 or 1)
	//heth.Init.PhyAddress ^= 0x1;
	printf("PHY Addr: %d\n", heth.Init.PhyAddress);
	if (HAL_ETH_ReadPHYRegister(&heth, 2, &phy_ident_1) == HAL_OK) {
		printf("PHY Ident 1: %08x\n", (unsigned) phy_ident_1);
		if (HAL_ETH_ReadPHYRegister(&heth, 3, &phy_ident_2) == HAL_OK) {
			printf("PHY Ident 2: %08x\n", (unsigned) phy_ident_2);
		} else {
			printf("Failed to read out PHY Identifier 2\n");
		}
	} else {
		printf("Failed to read out PHY Identifier 1\n");
	}

}

void start_ping_action() {
	//get gateway IP from global net interface
	ip_addr_t gw_addr = gnetif.gw;
	uint8_t gw_ip_part_1 = ip4_addr1(&gw_addr);
	//check if DHCP already succeeded by checking against non 0 ip part
	int ret = 0;
	if(gw_ip_part_1 != 0) {
		ip_addr_t target_ping_ip;
		//select target:
		//gateway
		//target_ping_ip = gw_addr;
		//static IPs
		//IP_ADDR4(&target_ping_ip, 192,168,1,180);
		IP_ADDR4(&target_ping_ip, 216,58,213,195); //google.com

		printf("Starting to ping IP: %d.%d.%d.%d.\n", (int)ip4_addr1(&target_ping_ip),
				(int)ip4_addr2(&target_ping_ip), (int)ip4_addr3(&target_ping_ip),
				(int)ip4_addr4(&target_ping_ip));
		if((ret = ping_ip(target_ping_ip)) != PING_ERR_OK) {
			printf("Error while sending ping: %d\n", ret);
		}
	}
	//every 4 seconds, start a new ping attempt
	sys_timeout(4000, start_ping_action, NULL);
}

void check_ping_result() {
	ping_result_t res;
	memset(&res, 0, sizeof(res));
	int retcode = 0;
	if((retcode = ping_ip_result(&res)) == PING_ERR_OK) {
        if (res.result_code == PING_RES_ECHO_REPLY) {
            printf("Good ping from %s %u ms\n", ipaddr_ntoa(&res.response_ip),
                    (unsigned) res.response_time_ms);
        } else {
            printf("Bad ping err %d\n", res.result_code);
        }
	} else {
		//printf("No ping result available yet: %d\n", retcode);
	}
	sys_timeout(100, check_ping_result, NULL);
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {
	/* USER CODE BEGIN 1 */

	/* USER CODE END 1 */

	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */

	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* USER CODE BEGIN SysInit */

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_USART1_UART_Init();
	MX_LWIP_Init();
	/* USER CODE BEGIN 2 */
	/* register timer function every 1 second for debugging */
	//sys_timeout(1000, check_debug_info, NULL);
	sys_timeout(4000, start_ping_action, NULL);
	sys_timeout(100, check_ping_result, NULL);
	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {
		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
		/* check ethernet data and feed into lwIP, if neccessary */
		MX_LWIP_Process();
	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

	/** Configure the main internal regulator output voltage
	 */
	__HAL_RCC_PWR_CLK_ENABLE()
	;
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
	/** Initializes the CPU, AHB and APB busses clocks
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLM = 8;
	RCC_OscInitStruct.PLL.PLLN = 168;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 4;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}
	/** Initializes the CPU, AHB and APB busses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
		Error_Handler();
	}
}

/**
 * @brief USART1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART1_UART_Init(void) {

	/* USER CODE BEGIN USART1_Init 0 */

	/* USER CODE END USART1_Init 0 */

	/* USER CODE BEGIN USART1_Init 1 */

	/* USER CODE END USART1_Init 1 */
	huart1.Instance = USART1;
	huart1.Init.BaudRate = 921600;
    //huart1.Init.BaudRate = 115200;
	huart1.Init.WordLength = UART_WORDLENGTH_8B;
	huart1.Init.StopBits = UART_STOPBITS_1;
	huart1.Init.Parity = UART_PARITY_NONE;
	huart1.Init.Mode = UART_MODE_TX_RX;
	huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart1.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart1) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN USART1_Init 2 */

	/* USER CODE END USART1_Init 2 */

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOH_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
}

/* USER CODE BEGIN 4 */
/* general hexdump function, useful. */
void hexDump (const char * desc, const void * addr, const int len) {
    int i;
    unsigned char buff[17];
    const unsigned char * pc = (const unsigned char *)addr;

    // Output description if given.

    if (desc != NULL)
        printf ("%s:\n", desc);

    // Length checks.

    if (len == 0) {
        printf("  ZERO LENGTH\n");
        return;
    }
    else if (len < 0) {
        printf("  NEGATIVE LENGTH: %d\n", len);
        return;
    }

    // Process every byte in the data.

    for (i = 0; i < len; i++) {
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0) {
            // Don't print ASCII buffer for the "zeroth" line.

            if (i != 0)
                printf ("  %s\n", buff);

            // Output the offset.

            printf ("  %04x ", i);
        }

        // Now the hex code for the specific character.
        printf (" %02x", pc[i]);

        // And buffer a printable ASCII character for later.

        if ((pc[i] < 0x20) || (pc[i] > 0x7e)) // isprint() may be better.
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.

    while ((i % 16) != 0) {
        printf ("   ");
        i++;
    }

    // And print the final ASCII buffer.

    printf ("  %s\n", buff);
}
/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */

	/* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
	/* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
	 tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	/* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
