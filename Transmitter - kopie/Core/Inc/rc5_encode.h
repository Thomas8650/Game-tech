/**
  ******************************************************************************
  * @file    rc5_encode.h
  * @author  MCD Application Team
  * @brief   This file contains all the functions prototypes for the rc5 encode
  *          firmware library.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __RC5_ENCODE_H
#define __RC5_ENCODE_H


/** @addtogroup IR_REMOTE
  * @{
  */

/** @addtogroup RC5_ENCODE
  * @{
  */

/** @addtogroup RC5_Public_Types
  * @{
  */

/**
  * @brief Definition of the RC5 Control bit value.
  */
typedef enum
{
  RC5_CTRL_RESET                        = ((uint16_t)0),
  RC5_CTRL_SET                          = ((uint16_t)0x0800)
}RC5_Ctrl_t;

/**
  * @}
  */

/** @addtogroup RC5_Public_Variables
  * @{
  */
extern const uint8_t* aRC5Commands [];
extern const uint8_t* aRC5Devices[];

/**
  * @}
  */

/** @addtogroup RC5_Exported_Functions
  * @{
  */
void Menu_RC5_Encode_Func(void);
void RC5_Encode_Init(void);
void RC5_Encode_SendFrame(uint8_t RC5_Address, uint8_t RC5_Instruction, RC5_Ctrl_t RC5_Ctrl);
void RC5_Encode_SignalGenerate(void);

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#endif  /*__RC5_ENCODE_H */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
