/**************************************************************************/ /**
 * @file      CliThread.c
 * @brief     File for the CLI Thread handler. Uses FREERTOS + CLI
 * @author    Eduardo Garcia
 * @date      2020-02-15

 ******************************************************************************/

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "CliThread.h"
#include "SerialConsole.h"
#include "semphr.h"
#include "FreeRTOS.h"
#include "task.h"
#include "string.h"
#include "stdio.h"
/******************************************************************************
 * Defines
 ******************************************************************************/
#define FIRMWARE_VERSION "0.0.1"  // Modify this to update the version

/******************************************************************************
 * Variables
 ******************************************************************************/
static int8_t *const pcWelcomeMessage =
    "FreeRTOS CLI.\r\nType Help to view a list of registered commands.\r\n";

// Clear screen command
const CLI_Command_Definition_t xClearScreen =
    {
        CLI_COMMAND_CLEAR_SCREEN,
        CLI_HELP_CLEAR_SCREEN,
        CLI_CALLBACK_CLEAR_SCREEN,
        CLI_PARAMS_CLEAR_SCREEN};

static const CLI_Command_Definition_t xResetCommand =
    {
        "reset",
        "reset: Resets the device\r\n",
        (const pdCOMMAND_LINE_CALLBACK)CLI_ResetDevice,
        0};


// Declare the command structures
static const CLI_Command_Definition_t xVersionCommand = {
    "version",                     // Command string (what you type in Tera Term)
    "\r\nversion:\r\n Prints the firmware version\r\n", // Help message
    CLI_GetVersion,                // Function that handles the command
    0                              // No parameters required
};

static const CLI_Command_Definition_t xTicksCommand = {
    "ticks",                       // Command string (what you type in Tera Term)
    "\r\nticks:\r\n Prints the number of system ticks\r\n", // Help message
    CLI_GetTicks,                  // Function that handles the command
    0                              // No parameters required
};

/*void vRegisterCLICommands(void)
{
    FreeRTOS_CLIRegisterCommand(&xVersionCommand);
    FreeRTOS_CLIRegisterCommand(&xTicksCommand);
}*/

/**************************************************************************/ /**
 * @fn			void FreeRTOS_read(char* character)
 * @brief		STUDENTS TO COMPLETE. This function block the thread unless we received a character. How can we do this?
                 There are multiple solutions! Check all the inter-thread communications available! See https://www.freertos.org/a00113.html
 * @details		STUDENTS TO COMPLETE.
 * @note
 *****************************************************************************/

/******************************************************************************
 * Forward Declarations
 ******************************************************************************/
static void FreeRTOS_read(char *character);

/******************************************************************************
 * Define Variables
 ******************************************************************************/

// char BufferArray[Circular_BufferSize];  // Define the actual buffer
// uint16_t Head = 0;                   // Define Head
// uint16_t Tail = 0;                   // Define Tail
// SemaphoreHandle_t rxSemaphore = NULL;  // Define rxSemaphore

// Define RX Semaphore
// SemaphoreHandle_t RX_Buffer;

/******************************************************************************
 * Callback Functions
 ******************************************************************************/
/* static void FreeRTOS_read(char *character){
	// If Head != Tail: No Available Character to read in Reception Buffer
	// Wait Data to Activate Thread
	while (Head == Tail)
	{
		xSemaphoreTake(RX_Buffer, portMAX_DELAY);
	}	
	
	*character = BufferArray[Tail];
	Tail = (Tail + 1) % Circular_BufferSize;
	
}*/

cbuf_handle_t cbufRx;

SemaphoreHandle_t RxMutex;

static void FreeRTOS_read(char *character) {
	// If character pointer is NULL, return early
	if (character == NULL) {
		return;  // Exit the function early if the pointer is invalid
	}
	
	/* // Wait until data is available in the buffer
	while (circular_buf_empty(cbuf)) {
		xSemaphoreTake(rxSemaphore, portMAX_DELAY);  // Wait indefinitely for the semaphore
	} */

	// Wait until data is available in the buffer
	if (xSemaphoreTake(rxSemaphore, portMAX_DELAY) == pdPASS) {
		// Once the semaphore is successfully taken, check if the buffer is not empty
		if (circular_buf_get(cbufRx, (uint8_t *)character) != 0) {
			// Fail to retrieve a character
			*character = '\0';
		}
		xSemaphoreGive(RxMutex);
	}
	// If semaphore take fails, nothing is done, or you can handle the failure here
}

/******************************************************************************
 * CLI Thread
 ******************************************************************************/

void vCommandConsoleTask(void *pvParameters)
{
    // REGISTER COMMANDS HERE

    FreeRTOS_CLIRegisterCommand(&xClearScreen);
    FreeRTOS_CLIRegisterCommand(&xResetCommand);

	FreeRTOS_CLIRegisterCommand(&xVersionCommand);
	FreeRTOS_CLIRegisterCommand(&xTicksCommand);
	
    uint8_t cRxedChar[2], cInputIndex = 0;
    BaseType_t xMoreDataToFollow;
    /* The input and output buffers are declared static to keep them off the stack. */
    static char pcOutputString[MAX_OUTPUT_LENGTH_CLI], pcInputString[MAX_INPUT_LENGTH_CLI];
    static char pcLastCommand[MAX_INPUT_LENGTH_CLI];
    static bool isEscapeCode = false;
    static char pcEscapeCodes[4];
    static uint8_t pcEscapeCodePos = 0;

    // Any semaphores/mutexes/etc you needed to be initialized, you can do them here

    /* This code assumes the peripheral being used as the console has already
    been opened and configured, and is passed into the task as the task
    parameter.  Cast the task parameter to the correct type. */

    /* Send a welcome message to the user knows they are connected. */
    SerialConsoleWriteString(pcWelcomeMessage);
    char rxChar;
    for (;;)
    {
        /* This implementation reads a single character at a time.  Wait in the
        Blocked state until a character is received. */

        FreeRTOS_read(&cRxedChar);

        if (cRxedChar[0] == '\n' || cRxedChar[0] == '\r')
        {
            /* A newline character was received, so the input command string is
            complete and can be processed.  Transmit a line separator, just to
            make the output easier to read. */
            SerialConsoleWriteString("\r\n");
            // Copy for last command
            isEscapeCode = false;
            pcEscapeCodePos = 0;
            strncpy(pcLastCommand, pcInputString, MAX_INPUT_LENGTH_CLI - 1);
            pcLastCommand[MAX_INPUT_LENGTH_CLI - 1] = 0; // Ensure null termination

            /* The command interpreter is called repeatedly until it returns
            pdFALSE.  See the "Implementing a command" documentation for an
            explanation of why this is. */
            do
            {
                /* Send the command string to the command interpreter.  Any
                output generated by the command interpreter will be placed in the
                pcOutputString buffer. */
                xMoreDataToFollow = FreeRTOS_CLIProcessCommand(
                    pcInputString,        /* The command string.*/
                    pcOutputString,       /* The output buffer. */
                    MAX_OUTPUT_LENGTH_CLI /* The size of the output buffer. */
                );

                /* Write the output generated by the command interpreter to the
                console. */
                // Ensure it is null terminated
                pcOutputString[MAX_OUTPUT_LENGTH_CLI - 1] = 0;
                SerialConsoleWriteString(pcOutputString);

            } while (xMoreDataToFollow != pdFALSE);

            /* All the strings generated by the input command have been sent.
            Processing of the command is complete.  Clear the input string ready
            to receive the next command. */
            cInputIndex = 0;
            memset(pcInputString, 0x00, MAX_INPUT_LENGTH_CLI);
        }
        else
        {
            /* The if() clause performs the processing after a newline character
    is received.  This else clause performs the processing if any other
    character is received. */

            if (true == isEscapeCode)
            {

                if (pcEscapeCodePos < CLI_PC_ESCAPE_CODE_SIZE)
                {
                    pcEscapeCodes[pcEscapeCodePos++] = cRxedChar[0];
                }
                else
                {
                    isEscapeCode = false;
                    pcEscapeCodePos = 0;
                }

                if (pcEscapeCodePos >= CLI_PC_MIN_ESCAPE_CODE_SIZE)
                {

                    // UP ARROW SHOW LAST COMMAND
                    if (strcasecmp(pcEscapeCodes, "oa"))
                    {
                        /// Delete current line and add prompt (">")
                        sprintf(pcInputString, "%c[2K\r>", 27);
                        SerialConsoleWriteString(pcInputString);
                        /// Clear input buffer
                        cInputIndex = 0;
                        memset(pcInputString, 0x00, MAX_INPUT_LENGTH_CLI);
                        /// Send last command
                        strncpy(pcInputString, pcLastCommand, MAX_INPUT_LENGTH_CLI - 1);
                        cInputIndex = (strlen(pcInputString) < MAX_INPUT_LENGTH_CLI - 1) ? strlen(pcLastCommand) : MAX_INPUT_LENGTH_CLI - 1;
                        SerialConsoleWriteString(pcInputString);
                    }

                    isEscapeCode = false;
                    pcEscapeCodePos = 0;
                }
            }
            /* The if() clause performs the processing after a newline character
            is received.  This else clause performs the processing if any other
            character is received. */

            else if (cRxedChar[0] == '\r')
            {
                /* Ignore carriage returns. */
            }
            else if (cRxedChar[0] == ASCII_BACKSPACE || cRxedChar[0] == ASCII_DELETE)
            {
                char erase[4] = {0x08, 0x20, 0x08, 0x00};
                SerialConsoleWriteString(erase);
                /* Backspace was pressed.  Erase the last character in the input
                buffer - if there are any. */
                if (cInputIndex > 0)
                {
                    cInputIndex--;
                    pcInputString[cInputIndex] = 0;
                }
            }
            // ESC
            else if (cRxedChar[0] == ASCII_ESC)
            {
                isEscapeCode = true; // Next characters will be code arguments
                pcEscapeCodePos = 0;
            }
            else
            {
                /* A character was entered.  It was not a new line, backspace
                or carriage return, so it is accepted as part of the input and
                placed into the input buffer.  When a n is entered the complete
                string will be passed to the command interpreter. */
                if (cInputIndex < MAX_INPUT_LENGTH_CLI)
                {
                    pcInputString[cInputIndex] = cRxedChar[0];
                    cInputIndex++;
                }

                // Order Echo
                cRxedChar[1] = 0;
                SerialConsoleWriteString(&cRxedChar[0]);
            }
        }
    }
}


/******************************************************************************
 * CLI Functions - Define here
 ******************************************************************************/

// THIS COMMAND USES vt100 TERMINAL COMMANDS TO CLEAR THE SCREEN ON A TERMINAL PROGRAM LIKE TERA TERM
// SEE http://www.csie.ntu.edu.tw/~r92094/c++/VT100.html for more info
// CLI SPECIFIC COMMANDS
static char bufCli[CLI_MSG_LEN];
BaseType_t xCliClearTerminalScreen(char *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString)
{
    char clearScreen = ASCII_ESC;
    snprintf(bufCli, CLI_MSG_LEN - 1, "%c[2J", clearScreen);
    snprintf(pcWriteBuffer, xWriteBufferLen, bufCli);
    return pdFALSE;
}

// Example CLI Command. Resets system.
BaseType_t CLI_ResetDevice(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString)
{
    system_reset();
    return pdFALSE;
}

/**
 * @brief Retrieves the firmware version and writes it to the output buffer.
 *
 * This function writes the firmware version string to the provided buffer.
 * The firmware version is defined by the macro `FIRMWARE_VERSION`.
 *
 * @param[out] pcWriteBuffer Pointer to the buffer where the output string will be stored.
 * @param[in] xWriteBufferLen Size of the output buffer.
 * @param[in] pcCommandString Pointer to the command string (unused in this function).
 *
 * @return pdFALSE to indicate that no further output is required.
 */
BaseType_t CLI_GetVersion(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString)
{
	snprintf(pcWriteBuffer, xWriteBufferLen, "Firmware Version: %s\r\n", FIRMWARE_VERSION);
	return pdFALSE;
}

/**
 * @brief Retrieves the number of ticks since the FreeRTOS scheduler started.
 *
 * This function uses the FreeRTOS API `xTaskGetTickCount()` to get the system tick count
 * and writes it to the provided buffer.
 *
 * @param[out] pcWriteBuffer Pointer to the buffer where the output string will be stored.
 * @param[in] xWriteBufferLen Size of the output buffer.
 * @param[in] pcCommandString Pointer to the command string (unused in this function).
 *
 * @return pdFALSE to indicate that no further output is required.
 */
BaseType_t CLI_GetTicks(int8_t *pcWriteBuffer, size_t xWriteBufferLen, const int8_t *pcCommandString)
{
	TickType_t ticks = xTaskGetTickCount(); // FreeRTOS API: Get Ticks
	snprintf(pcWriteBuffer, xWriteBufferLen, "System Ticks: %lu\r\n", (unsigned long)ticks);
	return pdFALSE;
}


