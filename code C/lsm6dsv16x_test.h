/*
 * lsm6dsv16x_test.h
 * Test minimal du LSM6DSV16X (carte STEVAL-MKI227KA) sur STM32F410 (NUCLEO-F410RB).
 * I2C1 (PB8 = SCL / D15, PB9 = SDA / D14), affichage sur USART2 (VCP du ST-LINK, 115200 8N1).
 */
#ifndef LSM6DSV16X_TEST_H
#define LSM6DSV16X_TEST_H

#include "stm32f4xx_hal.h"

/* Lance le test : scan I2C, lecture WHO_AM_I, configuration, puis boucle infinie
 * qui affiche accéléromètre (mg), gyroscope (mdps) et température (centièmes de °C).
 * Ne retourne pas, sauf en cas d'erreur (capteur introuvable). */
void LSM6_Test_Run(I2C_HandleTypeDef *hi2c, UART_HandleTypeDef *huart);

#endif
