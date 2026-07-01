/**
  * \file    errors.h
  * \version V1.0.0
  * \date    21 августа 2020
  * \brief   Функия assert и ошибки
  */


#ifndef ERRORS_H_
#define ERRORS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief  The assert_param macro is used for function's parameters check.
* @param  expr: If expr is false, it calls assert_failed user's function which
*         reports the name of the source file, source line number
*         and expression text (if USE_ASSERT_INFO == 2) of the call that failed.
*         That function should not return. If expr is true, nothing is done.
* @retval None
*/
#if (USE_ASSERT_INFO == 0)
#define assert_param(expr) ((void)0U)
#elif (USE_ASSERT_INFO == 1)
#define assert_param(expr) ((expr) ? (void)0U : assert_failed((uint8_t *)__FILE__, __LINE__))
    void assert_failed(uint8_t* file, uint32_t line);
#elif (USE_ASSERT_INFO == 2)
#define assert_param(expr) ((expr) ? (void)0U : assert_failed((uint8_t *)__FILE__, __LINE__, (const uint8_t*) #expr))
void assert_failed(uint8_t* file, uint32_t line, const uint8_t* expr);
#else
#error "Unsupported USE_ASSERT_INFO level"
#endif /* USE_ASSERT_INFO */


void Error_Handler(void);

#if USE_ASSERT_INFO == 1
void assert_failed(uint8_t *file, uint32_t line);
#elif USE_ASSERT_INFO == 2
void assert_failed(uint8_t* file, uint32_t line, const uint8_t* expr);
#endif

#ifdef __cplusplus
}
#endif


#endif /* ERRORS_H_ */
