/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Proyecto 2 - Nucleo maestro UART / SPI / I2C (version con funciones)
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
  ESTADO_MENU,         // Esperando opcion 1 o 2
  ESTADO_SUBMENU_SPI   // Esperando comando "led,tiempo"
} EstadoMenu;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define DIRECCION_ESP32_I2C  (0x28 << 1)  // HAL usa la direccion desplazada
#define COMANDO_I2C_LEER_POT 0x01         // comando "dame una medicion"
#define ENCABEZADO_SPI       0xAA         // inicio de la trama SPI
#define LARGO_TRAMA_SPI      4            // [0xAA][led][ms alto][ms bajo]
#define LED_MAXIMO           3
#define TIEMPO_MAXIMO_MS     60000
#define LARGO_BUFFER_UART    16
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
EstadoMenu estadoActual = ESTADO_MENU;
char     lineaRecibida[LARGO_BUFFER_UART];
uint8_t  posicionLinea = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */
static void UART_Print(const char *texto);
static uint8_t UART_LeerLinea(void);
static void MostrarMenuPrincipal(void);
static void MostrarMenuSPI(void);
static uint8_t ValidarComando(const char *texto, uint8_t *led, uint16_t *tiempoMs);
static HAL_StatusTypeDef SPI_EnviarComando(uint8_t led, uint16_t tiempoMs);
static HAL_StatusTypeDef I2C_LeerPotenciometro(uint8_t *valor);
static void I2C_Reiniciar(void);
static void I2C_Escanear(void);
static void AccionSPI(const char *texto);
static void AccionI2C(void);
static void ProcesarEstado(void);
static void Timer_ActualizarPulso(uint16_t tiempoMs);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

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
  MX_SPI1_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  HAL_GPIO_WritePin(ESP32_GPIO_Port, ESP32_Pin, GPIO_PIN_SET);  // CS en reposo (alto)
  I2C_Escanear();          // muestra que dispositivos I2C encuentra (debe salir 0x28)
  MostrarMenuPrincipal();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    ProcesarEstado();
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_IC_InitTypeDef sConfigIC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 8399;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 4294967295;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_IC_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 0;
  if (HAL_TIM_IC_ConfigChannel(&htim2, &sConfigIC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, ledpulso_Pin|ESP32_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : ledpulso_Pin ESP32_Pin */
  GPIO_InitStruct.Pin = ledpulso_Pin|ESP32_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* ------------------------------ UART ------------------------------------- */

// Envia un texto por UART2 a la terminal
static void UART_Print(const char *texto)
{
  HAL_UART_Transmit(&huart2, (uint8_t *)texto, strlen(texto), HAL_MAX_DELAY);
}

// Lectura sin bloqueo: junta caracteres y devuelve 1 cuando se presiona Enter
static uint8_t UART_LeerLinea(void)
{
  if (!__HAL_UART_GET_FLAG(&huart2, UART_FLAG_RXNE)) return 0;

  uint8_t caracter = (uint8_t)(huart2.Instance->DR & 0xFF);

  if (caracter == '\r' || caracter == '\n') {
    if (posicionLinea == 0) return 0;      // ignora el segundo caracter de "\r\n"
    lineaRecibida[posicionLinea] = '\0';
    posicionLinea = 0;
    return 1;
  }
  if ((caracter == 0x08 || caracter == 0x7F) && posicionLinea > 0) {  // borrar
    posicionLinea--;
    UART_Print("\b \b");
    return 0;
  }
  if (posicionLinea < LARGO_BUFFER_UART - 1 && caracter >= ' ') {
    lineaRecibida[posicionLinea++] = caracter;
    HAL_UART_Transmit(&huart2, &caracter, 1, 10);  // eco
  }
  return 0;
}

/* ------------------------------ Menus ------------------------------------ */

static void MostrarMenuPrincipal(void)
{
  UART_Print("\r\n===== MENU PRINCIPAL =====\r\n"
             "1. Controlar dispositivo SPI\r\n"
             "2. Obtener medicion de sensor I2C\r\n"
             "Opcion: ");
  estadoActual = ESTADO_MENU;
}

static void MostrarMenuSPI(void)
{
  UART_Print("\r\n--- Dispositivo SPI ---\r\n"
             "Formato: led,tiempo_ms  (1=Rojo 2=Amarillo 3=Verde)\r\n"
             "Ejemplo: 1,500\r\n"
             "Comando: ");
  estadoActual = ESTADO_SUBMENU_SPI;
}

/* ------------------------------ Validacion ------------------------------- */

// Valida "led,tiempo". Devuelve 1 si es correcto y entrega led y tiempo.
static uint8_t ValidarComando(const char *texto, uint8_t *led, uint16_t *tiempoMs)
{
  char *final;
  char *inicio;

  long numeroLed = strtol(texto, &final, 10);
  if (final == texto) return 0;
  while (*final == ' ') final++;
  if (*final != ',') return 0;
  final++;

  inicio = final;
  long tiempo = strtol(inicio, &final, 10);
  if (final == inicio) return 0;
  while (*final == ' ') final++;
  if (*final != '\0') return 0;

  if (numeroLed < 1 || numeroLed > LED_MAXIMO) return 0;
  if (tiempo < 1 || tiempo > TIEMPO_MAXIMO_MS) return 0;

  *led = (uint8_t)numeroLed;
  *tiempoMs = (uint16_t)tiempo;
  return 1;
}

/* ------------------------------ SPI -------------------------------------- */

// Trama: [0xAA][led][ms alto][ms bajo]. CS en bajo durante toda la trama.
static HAL_StatusTypeDef SPI_EnviarComando(uint8_t led, uint16_t tiempoMs)
{
  uint8_t datosEnviar[LARGO_TRAMA_SPI] = { ENCABEZADO_SPI, led, (uint8_t)(tiempoMs >> 8), (uint8_t)(tiempoMs & 0xFF) };
  uint8_t datosRecibidos[LARGO_TRAMA_SPI];

  HAL_GPIO_WritePin(ESP32_GPIO_Port, ESP32_Pin, GPIO_PIN_RESET);
  HAL_StatusTypeDef resultado = HAL_SPI_TransmitReceive(&hspi1, datosEnviar, datosRecibidos, LARGO_TRAMA_SPI, 100);
  HAL_GPIO_WritePin(ESP32_GPIO_Port, ESP32_Pin, GPIO_PIN_SET);
  return resultado;
}

static void AccionSPI(const char *texto)
{
  uint8_t  led;
  uint16_t tiempoMs;
  char     mensaje[128];

  if (!ValidarComando(texto, &led, &tiempoMs)) {
    UART_Print("Comando invalido. Intente de nuevo.\r\n");
    MostrarMenuSPI();
    return;
  }

  Timer_ActualizarPulso(tiempoMs);   // Parte 5: actualiza el pulso con el mismo comando

  if (SPI_EnviarComando(led, tiempoMs) == HAL_OK) {
    snprintf(mensaje, sizeof(mensaje),
             "Enviado por SPI: LED %u durante %u ms\r\nPulso: %u ms alto, %u ms bajo\r\n",
             led, tiempoMs, tiempoMs, tiempoMs);
    UART_Print(mensaje);
  } else {
    UART_Print("Error en la comunicacion SPI.\r\n");
  }
  MostrarMenuPrincipal();
}

/* ------------------------------ I2C -------------------------------------- */

// Reinicia el periferico I2C (limpia el bus despues de un error)
static void I2C_Reiniciar(void)
{
  HAL_I2C_DeInit(&hi2c1);
  MX_I2C1_Init();
}

// Prueba todas las direcciones e imprime las que responden
static void I2C_Escanear(void)
{
  char    mensaje[40];
  uint8_t direccion = 1;
  uint8_t encontrados = 0;

  UART_Print("\r\nEscaneando I2C...\r\n");
  while (direccion < 127) {
    if (HAL_I2C_IsDeviceReady(&hi2c1, direccion << 1, 1, 10) == HAL_OK) {
      snprintf(mensaje, sizeof(mensaje), "Encontrado: 0x%02X\r\n", direccion);
      UART_Print(mensaje);
      encontrados++;
    }
    direccion++;
  }
  if (encontrados == 0) UART_Print("No se encontro ningun dispositivo.\r\n");
  UART_Print("Escaneo terminado.\r\n");
}

// Pide la medicion al ESP32 (escribe el comando) y lee 1 byte (0-255)
static HAL_StatusTypeDef I2C_LeerPotenciometro(uint8_t *valor)
{
  uint8_t comando = COMANDO_I2C_LEER_POT;

  if (HAL_I2C_Master_Transmit(&hi2c1, DIRECCION_ESP32_I2C, &comando, 1, 100) != HAL_OK) {
    UART_Print("Fallo al ESCRIBIR el comando.\r\n");
    return HAL_ERROR;
  }
  if (HAL_I2C_Master_Receive(&hi2c1, DIRECCION_ESP32_I2C, valor, 1, 100) != HAL_OK) {
    UART_Print("Fallo al LEER el dato.\r\n");
    return HAL_ERROR;
  }
  return HAL_OK;
}

static void AccionI2C(void)
{
  uint8_t valor;
  char    mensaje[64];

  if (I2C_LeerPotenciometro(&valor) == HAL_OK) {
    uint32_t milivoltios = (uint32_t)valor * 3300U / 255U;   // sin printf de float
    snprintf(mensaje, sizeof(mensaje), "Pot1: %u (0-255) = %lu.%02lu V\r\n",
             valor, (unsigned long)(milivoltios / 1000), (unsigned long)((milivoltios % 1000) / 10));
  } else {
    snprintf(mensaje, sizeof(mensaje), "Error I2C. Codigo: 0x%02lX\r\n",
             (unsigned long)HAL_I2C_GetError(&hi2c1));
    I2C_Reiniciar();
  }
  UART_Print(mensaje);
}

/* ------------------------------ Maquina de estados ----------------------- */

static void ProcesarEstado(void)
{
  if (!UART_LeerLinea()) return;
  UART_Print("\r\n");

  switch (estadoActual) {
    case ESTADO_MENU:
      if (strcmp(lineaRecibida, "1") == 0) {
        MostrarMenuSPI();
      } else if (strcmp(lineaRecibida, "2") == 0) {
        AccionI2C();
        MostrarMenuPrincipal();
      } else {
        UART_Print("Opcion invalida.\r\n");
        MostrarMenuPrincipal();
      }
      break;

    case ESTADO_SUBMENU_SPI:
      AccionSPI(lineaRecibida);
      break;
  }
}

// Cambia el tiempo en alto y en bajo del pulso sin detener el programa
static void Timer_ActualizarPulso(uint16_t tiempoMs)
{
  uint32_t ticks = (uint32_t)tiempoMs * 10;        // 1 tick = 0.1 ms

  __HAL_TIM_SET_AUTORELOAD(&htim2, ticks - 1);
  __HAL_TIM_SET_COUNTER(&htim2, 0);
  HAL_GPIO_WritePin(ledpulso_GPIO_Port, ledpulso_Pin, GPIO_PIN_SET);  // empieza en alto
  HAL_TIM_Base_Start_IT(&htim2);
}

// Cada vez que se cumple el tiempo, el pin cambia de estado
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM2) {
    HAL_GPIO_TogglePin(ledpulso_GPIO_Port, ledpulso_Pin);
  }
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
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
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
