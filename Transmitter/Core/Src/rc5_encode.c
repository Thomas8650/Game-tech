/**
  ******************************************************************************
  * @file    rc5_encode.c
  * @author  MCD Application Team
  * @brief   This file provides all the rc5 encode firmware functions
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics. 
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the 
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "rc5_encode.h"

/* Private_Defines -----------------------------------------------------------*/
#define  RC5HIGHSTATE     ((uint8_t )0x01)   /* RC5 high level definition - changed to 01 */
#define  RC5LOWSTATE      ((uint8_t )0x02)   /* RC5 low level definition - changed to 10 */

/* Private_Function_Prototypes -----------------------------------------------*/
static uint16_t RC5_BinFrameGeneration(uint8_t RC5_Address, uint8_t RC5_Instruction, RC5_Ctrl_t RC5_Ctrl);
static uint32_t RC5_ManchesterConvert(uint16_t RC5_BinaryFrameFormat);

/* Private_Variables ---------------------------------------------------------*/
uint8_t RC5RealFrameLength = 14;
uint8_t RC5GlobalFrameLength = 64;
uint16_t RC5BinaryFrameFormat = 0;
uint32_t RC5ManchesterFrameFormat = 0;
__IO uint32_t RC5SendOpCompleteFlag = 1;
__IO uint32_t RC5SendOpReadyFlag = 0;
__IO uint8_t BitsSentCounter = 0;
__IO uint32_t DebugISRCounter = 0;  /* DEBUG: Count ISR calls */
__IO uint32_t DebugLastCount = 0;   /* DEBUG: Store last transmission count */
RC5_Ctrl_t RC5Ctrl1 = RC5_CTRL_RESET;

/* Timer handles (external, defined in main.c) */
extern TIM_HandleTypeDef htim15;  /* LF envelope timer */
extern TIM_HandleTypeDef htim16;  /* HF carrier timer (38kHz) */

/* Exported_Functions--------------------------------------------------------*/

/**
  * @brief  RCR transmitter demo (simplified for NUCLEO-L432KC)
  * @param  None
  * @retval None
  */
void Menu_RC5_Encode_Func(void)
{
  /* Simplified version without LCD - kept for compatibility */
  /* Use RC5_Encode_Init() and RC5_Encode_SendFrame() directly instead */
}

/**
  * @brief Init Hardware (IPs used) for RC5 generation
  * @param None
  * @retval  None
  */
void RC5_Encode_Init(void)
{
  /* TIM15 and TIM16 are already configured by CubeMX */
  /* TIM15: LF envelope (889us half-bit period) */
  /* TIM16: HF carrier (38kHz with 25% duty cycle) */
  
  /* Make sure TIM15 is stopped initially */
  HAL_TIM_Base_Stop_IT(&htim15);

  /* Start TIM16 carrier oscillator once and keep it running continuously.     */
  /* Carrier on/off is controlled by CCR: 210 = 25% duty, 0 = output stays low */
  __HAL_TIM_SET_COMPARE(&htim16, TIM_CHANNEL_1, 0);  /* Start with carrier OFF */
  HAL_TIM_PWM_Start(&htim16, TIM_CHANNEL_1);
  
  /* Reset bit counter */
  BitsSentCounter = 0;
  RC5SendOpReadyFlag = 0;
  RC5SendOpCompleteFlag = 1;
}

/**
  * @brief Generate and Send the RC5 frame.
  * @param RC5_Address : the RC5 Device destination
  * @param RC5_Instruction : the RC5 command instruction
  * @param RC5_Ctrl : the RC5 Control bit.
  * @retval  None
  */
void RC5_Encode_SendFrame(uint8_t RC5_Address, uint8_t RC5_Instruction, RC5_Ctrl_t RC5_Ctrl)
{
  /* Wait for previous transmission to complete */
  while (RC5SendOpCompleteFlag == 0x00)
  {}
  
  /* Generate a binary format of the Frame */
  RC5BinaryFrameFormat = RC5_BinFrameGeneration(RC5_Address, RC5_Instruction, RC5_Ctrl);

  /* Generate a Manchester format of the Frame */
  RC5ManchesterFrameFormat = RC5_ManchesterConvert(RC5BinaryFrameFormat);

  /* Reset bit counter explicitly before each new transmission */
  BitsSentCounter = 0;
  DebugISRCounter = 0;

  /* Reset the counter to ensure accurate timing of sync pulse */
  __HAL_TIM_SET_COUNTER(&htim15, 0);

  /* Set the Send operation Ready flag LAST, right before starting timer */
  RC5SendOpReadyFlag = 1;

  /* TIM IT Enable - start transmission */
  HAL_TIM_Base_Start_IT(&htim15);
}

/**
  * @brief Send by hardware Manchester Format RC5 Frame.
  * @retval None
  */
void RC5_Encode_SignalGenerate(void)
{
  uint32_t bit_msg = 0;

  if ((RC5SendOpReadyFlag == 1) && (BitsSentCounter < (RC5RealFrameLength * 2)))
  {
    DebugISRCounter++;  /* DEBUG: Count only data bits sent */
    RC5SendOpCompleteFlag = 0x00;
    /* Read bits MSB first: from bit 27 down to bit 0 */
    bit_msg = (uint8_t)((RC5ManchesterFrameFormat >> ((RC5RealFrameLength * 2 - 1) - BitsSentCounter)) & 1);

    if (bit_msg == 1)
    {
      /* Enable carrier: set CCR to 210 (25% duty cycle at 38 kHz) */
      /* TIM16 keeps running - only the compare value changes */
      __HAL_TIM_SET_COMPARE(&htim16, TIM_CHANNEL_1, 210);
    }
    else
    {
      /* Disable carrier: set CCR to 0 (output stays LOW) */
      __HAL_TIM_SET_COMPARE(&htim16, TIM_CHANNEL_1, 0);
    }
    BitsSentCounter++;
  }
  else
  {
    RC5SendOpCompleteFlag = 0x01;

    /* Carrier off: set CCR to 0 (TIM16 keeps running) */
    __HAL_TIM_SET_COMPARE(&htim16, TIM_CHANNEL_1, 0);

    /* TIM15 IT Disable */
    HAL_TIM_Base_Stop_IT(&htim15);
    RC5SendOpReadyFlag = 0;
    
    /* DEBUG: Store count for inspection */
    DebugLastCount = DebugISRCounter;
    
    /* DEBUG: Show result via LED */
    if (DebugISRCounter >= 26 && DebugISRCounter <= 30)
    {
      /* Close enough - probably working */
      HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_SET);
    }
    else
    {
      /* Way off - something wrong */
      HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);
    }
    DebugISRCounter = 0;
    
    BitsSentCounter = 0;
  }
}


/* Private functions ---------------------------------------------------------*/

/**
  * @brief Generate the binary format of the RC5 frame.
  * @param RC5_Address : Select the device address.
  * @param RC5_Instruction : Select the device instruction.
  * @param RC5_Ctrl : Select the device control bit status.
  * @retval Binary format of the RC5 Frame.
  */
static uint16_t RC5_BinFrameGeneration(uint8_t RC5_Address, uint8_t RC5_Instruction, RC5_Ctrl_t RC5_Ctrl)
{
  uint16_t star1 = 0x2000;
  uint16_t star2 = 0x1000;
  uint16_t addr = 0;

  /* Check if Instruction is 128-bit length */
  if (RC5_Instruction >= 64)
  {
    /* Reset field bit: command is 7-bit length */
    star2 = 0;
    /* Keep the lowest 6 bits of the command */
    RC5_Instruction &= 0x003F;
  }
  else /* Instruction is 64-bit length */
  {
    /* Set field bit: command is 6-bit length */
    star2 = 0x1000;
  }

  addr = ((uint16_t)(RC5_Address)) << 6;
  RC5BinaryFrameFormat =  (star1) | (star2) | (RC5_Ctrl) | (addr) | (RC5_Instruction);
  return (RC5BinaryFrameFormat);
}

/**
  * @brief Convert the RC5 frame from binary to Manchester Format.
  * @param RC5_BinaryFrameFormat : the RC5 frame in binary format.
  * @retval the RC5 frame in Manchester format.
  */
static uint32_t RC5_ManchesterConvert(uint16_t RC5_BinaryFrameFormat)
{
  uint8_t i = 0;
  uint16_t bit_format = 0;
  uint32_t ConvertedMsg = 0;

  /* Read bits MSB first (from bit 13 down to bit 0) for correct RC5 transmission order */
  for (i = 0; i < RC5RealFrameLength; i++)
  {
    /* Extract bit from MSB to LSB: bit (RC5RealFrameLength-1-i) */
    bit_format = (RC5BinaryFrameFormat >> (RC5RealFrameLength - 1 - i)) & 1;
    ConvertedMsg = ConvertedMsg << 2;

    if (bit_format != 0 ) /* Manchester 1 -|_  */
    {
      ConvertedMsg |= RC5HIGHSTATE;  /* 0x02 = binary 10 */
    }
    else /* Manchester 0 _|-  */
    {
      ConvertedMsg |= RC5LOWSTATE;   /* 0x01 = binary 01 */
    }
  }
  return (ConvertedMsg);
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
