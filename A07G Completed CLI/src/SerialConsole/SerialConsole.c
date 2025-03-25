/**************************************************************************/
/**
 * @file        SerialConsole.c
 * @ingroup 	Serial Console
 * @brief       This file has the code necessary to run the CLI and Serial Debugger. 
 * 				It initializes an UART channel and uses it to receive command from the user
 *				as well as print debug information.
 * @details     This file has the code necessary to run the CLI and Serial Debugger. 
 * 				It initializes an UART channel and uses it to receive command from the user
 *				as well as print debug information.
 *
 *				The code in this file will:
 *				--Initialize a SERCOM port (SERCOM # ) to be an UART channel operating at 115200 baud/second, 8N1
 *				--Register callbacks for the device to read and write characters asynchronously as required by the CLI
 *				--Initialize the CLI and Debug Logger data structures
 *
 *				Usage:
 *
 *
 * @copyright
 * @author
 * @date        January 26, 2019
 * @version		0.1
 *****************************************************************************/

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "SerialConsole.h"  // Includes RING_BUFFER_SIZE
// #include "CliThread.h"

// char BufferArray[Circular_BufferSize];  // Define the actual buffer
// uint16_t Head = 0;                   // Define Head
// uint16_t Tail = 0;                   // Define Tail
// SemaphoreHandle_t rxSemaphore = NULL;  // Define rxSemaphore

/******************************************************************************
 * Defines
 ******************************************************************************/
#define RX_BUFFER_SIZE 512 ///< Size of character buffer for RX, in bytes
#define TX_BUFFER_SIZE 512 ///< Size of character buffers for TX, in bytes

/******************************************************************************
 * Structures and Enumerations
 ******************************************************************************/
cbuf_handle_t cbufRx; ///< Circular buffer handler for receiving characters
cbuf_handle_t cbufTx; ///< Circular buffer handler for transmitting characters

char latestRx; ///< Holds the latest character received
char latestTx; ///< Holds the latest character to be transmitted

/******************************************************************************
 * Callback Declarations
 ******************************************************************************/
void usart_write_callback(struct usart_module *const usart_module); // Callback for when we finish writing characters to UART
void usart_read_callback(struct usart_module *const usart_module); // Callback for when we finis reading characters from UART

/******************************************************************************
 * Local Function Declarations
 ******************************************************************************/
static void configure_usart(void);
static void configure_usart_callbacks(void);

/******************************************************************************
 * Global Variables
 ******************************************************************************/
struct usart_module usart_instance;
char rxCharacterBuffer[RX_BUFFER_SIZE]; 			   ///< Buffer to store received characters
char txCharacterBuffer[TX_BUFFER_SIZE]; 			   ///< Buffer to store characters to be sent
enum eDebugLogLevels currentDebugLevel = LOG_INFO_LVL; ///< Default debug level

SemaphoreHandle_t rxSemaphore;  // Semaphore to notify when data is available

/******************************************************************************
 * Global Functions
 ******************************************************************************/

/**
 * @brief Initializes the UART and registers callbacks.
 */
void InitializeSerialConsole(void)
{
	rxSemaphore = xSemaphoreCreateBinary();
	
	if (rxSemaphore != NULL){
		xSemaphoreTake(rxSemaphore, 0);
	}
	
	RxMutex = xSemaphoreCreateMutex();
	
	 // Initialize circular buffers for RX and TX
	 cbufRx = circular_buf_init((uint8_t *)rxCharacterBuffer, RX_BUFFER_SIZE);
	 cbufTx = circular_buf_init((uint8_t *)txCharacterBuffer, TX_BUFFER_SIZE);

    // Configure USART and Callbacks
	configure_usart();
    configure_usart_callbacks();
    NVIC_SetPriority(SERCOM4_IRQn, 10);

    usart_read_buffer_job(&usart_instance, (uint8_t *)&latestRx, 1); // Kicks off constant reading of characters

	// Add any other calls you need to do to initialize your Serial Console
}

/**
 * @brief Deinitializes the UART.
 */
void DeinitializeSerialConsole(void)
{
    usart_disable(&usart_instance);
}

/**
 * @brief Writes a string to the UART.
 * @param string Pointer to the string to send.
 */
void SerialConsoleWriteString(char *string)
{
    if (string != NULL)
    {
        for (size_t iter = 0; iter < strlen(string); iter++)
        {
            circular_buf_put(cbufTx, string[iter]);
        }

        if (usart_get_job_status(&usart_instance, USART_TRANSCEIVER_TX) == STATUS_OK)
        {
            circular_buf_get(cbufTx, (uint8_t *)&latestTx); // Perform only if the SERCOM TX is free (not busy)
            usart_write_buffer_job(&usart_instance, (uint8_t *)&latestTx, 1);
        }
    }
}

/**
 * @brief Reads a character from the RX buffer.
 * @param rxChar Pointer to store the received character.
 * @return -1 if buffer is empty, otherwise the character read.
 */
int SerialConsoleReadCharacter(uint8_t *rxChar)
{
    vTaskSuspendAll();
    int a = circular_buf_get(cbufRx, (uint8_t *)rxChar);
    xTaskResumeAll();
    return a;
}

/**
 * @brief Gets the current debug log level.
 * @return The current debug level.
 */
enum eDebugLogLevels getLogLevel(void)
{
    return currentDebugLevel;
}

/**
 * @brief Sets the debug log level.
 * @param debugLevel The debug level to set.
 */
void setLogLevel(enum eDebugLogLevels debugLevel)
{
    currentDebugLevel = debugLevel;
}

/** Global variable declaration */
enum eDebugLogLevels currentLogLevel; // Ensure this is accessible globally

/**
 * @brief Logs a message at the specified debug level.
 * @param level The log level of the message.
 * @param format The format string (like printf).
 * @param ... Variable arguments for the format string.
 */
void LogMessage(enum eDebugLogLevels level, const char *format, ...) {
    if (level < currentLogLevel) {
        return; // Skip logging if the level is below the current threshold
    }

    char buffer[256]; // Buffer to hold the formatted message
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    SerialConsoleWriteString(buffer); // Send the formatted message to the console
}


/*
COMMAND LINE INTERFACE COMMANDS
*/

/******************************************************************************
 * Local Functions
 ******************************************************************************/

/**************************************************************************/ 
/**
 * @fn			static void configure_usart(void)
 * @brief		Code to configure the SERCOM "EDBG_CDC_MODULE" to be a UART channel running at 115200 8N1
 * @note
 *****************************************************************************/
static void configure_usart(void)
{
	struct usart_config config_usart;
	usart_get_config_defaults(&config_usart);

	config_usart.baudrate = 115200;
	config_usart.mux_setting = EDBG_CDC_SERCOM_MUX_SETTING;
	config_usart.pinmux_pad0 = EDBG_CDC_SERCOM_PINMUX_PAD0;
	config_usart.pinmux_pad1 = EDBG_CDC_SERCOM_PINMUX_PAD1;
	config_usart.pinmux_pad2 = EDBG_CDC_SERCOM_PINMUX_PAD2;
	config_usart.pinmux_pad3 = EDBG_CDC_SERCOM_PINMUX_PAD3;
	while (usart_init(&usart_instance,
					  EDBG_CDC_MODULE,
					  &config_usart) != STATUS_OK)
	{
	}

	usart_enable(&usart_instance);
}

/**************************************************************************/ 
/**
 * @fn			static void configure_usart_callbacks(void)
 * @brief		Code to register callbacks
 * @note
 *****************************************************************************/
static void configure_usart_callbacks(void)
{
	usart_register_callback(&usart_instance,
							usart_write_callback,
							USART_CALLBACK_BUFFER_TRANSMITTED);
	usart_register_callback(&usart_instance,
							usart_read_callback,
							USART_CALLBACK_BUFFER_RECEIVED);
	usart_enable_callback(&usart_instance, USART_CALLBACK_BUFFER_TRANSMITTED);
	usart_enable_callback(&usart_instance, USART_CALLBACK_BUFFER_RECEIVED);
}

/******************************************************************************
 * Callback Functions
 ******************************************************************************/

/**************************************************************************/ 
/**
 * @fn			void usart_read_callback(struct usart_module *const usart_module)
 * @brief		Callback called when the system finishes receives all the bytes requested from a UART read job
		 Students to fill out. Please note that the code here is dummy code. It is only used to show you how some functions work.
 * @note
 *****************************************************************************/
/* void usart_read_callback(struct usart_module *const usart_module) {
	uint8_t receivedChar;

	// Read the received character from UART hardware
	usart_read_buffer_job(usart_module, &receivedChar, 1);

	// Store in circular buffer
	taskENTER_CRITICAL();  // Ensure thread safety
	uint16_t nextHead = (Head + 1) % Circular_BufferSize;

	if (nextHead != Tail) {  // Check if buffer is full
		BufferArray[Head] = receivedChar;
		Head = nextHead;  // Move head forward
	}
	taskEXIT_CRITICAL();

	// Notify CLI thread a new character is available
	xSemaphoreGiveFromISR(rxSemaphore, NULL);
} */

// Assume the circular buffer handle and the semaphore are declared globally
// cbuf_handle_t cbuf;   // Circular buffer handle

SemaphoreHandle_t RxMutex;

void usart_read_callback(struct usart_module *const usart_module) {
	// uint8_t receivedChar;
	// Notify the FreeRTOS task that a new character is available
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	
	if (xSemaphoreTakeFromISR(RxMutex, &xHigherPriorityTaskWoken) == pdTRUE)
	{
		if (circular_buf_put2(cbufRx, latestRx) == 0){
			xSemaphoreGiveFromISR(rxSemaphore, &xHigherPriorityTaskWoken); // Signal that data is available
		}
		xSemaphoreGiveFromISR(RxMutex, &xHigherPriorityTaskWoken); // Signal that data is available
	}
	
	// Safety Loop: usart_read_buffer_job()
	while (usart_read_buffer_job(&usart_instance, (uint8_t *)&latestRx, 1) != STATUS_OK){
		// wait;
	}
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);  // Yield to a higher priority task if needed
} 





/**************************************************************************/ 
/**
 * @fn			void usart_write_callback(struct usart_module *const usart_module)
 * @brief		Callback called when the system finishes sending all the bytes requested from a UART read job
 * @note
 *****************************************************************************/
void usart_write_callback(struct usart_module *const usart_module)
{
	if (circular_buf_get(cbufTx, (uint8_t *)&latestTx) != -1) // Only continue if there are more characters to send
	{
		usart_write_buffer_job(&usart_instance, (uint8_t *)&latestTx, 1);
	}
}




