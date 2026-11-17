#!/usr/bin/env python3
"""Adapte le projet ST67W6X_CLI de la NUCLEO-N657X0-Q au STM32N6570-DK (MB1939).

Usage (depuis la racine du repo x-cube-st67w61 cloné) :
    python3 port_STM32N6570-DK.py Projects/NUCLEO-N657X0-Q/Applications/ST67W6X/ST67W6X_CLI

Le script copie le projet dans <projet>_DK puis modifie les sources C (pas le .ioc :
ne pas régénérer avec CubeMX après coup, ou refaire les mêmes changements dans le .ioc).

Différences de brochage Arduino entre les deux cartes (schémas ST) :
    signal X-NUCLEO   Arduino   NUCLEO-N657X0-Q   STM32N6570-DK
    SPI_MISO          D12       PG1               PH8   <- change
    BOOT              D6        PD5               PE13  <- change
    USER_BUTTON       D8        PD12              PE7   <- change (EXTI 7)
    SPI_SCK/MOSI/CS   D13/11/10 PE15/PG2/PA3      idem
    SPI_RDY           D3        PE9               idem
    CHIP_EN           D5        PE10              idem
    Console           ST-LINK   USART1 PE5/PE6    idem
Attention : sur le DK, PD5 est D1 (UART RX du module). Le firmware Nucleo y pilote BOOT,
d'où ce portage.
"""
import pathlib, re, shutil, sys

src = pathlib.Path(sys.argv[1]).resolve()
dst = src.with_name(src.name + "_DK")
if dst.exists():
    sys.exit(f"{dst} existe déjà")
shutil.copytree(src, dst)
F = dst / "FSBL"

def patch(path, pairs):
    t = path.read_text()
    for old, new in pairs:
        if old not in t:
            sys.exit(f"Motif introuvable dans {path} :\n{old}")
        t = t.replace(old, new)
    path.write_text(t)
    print("modifié :", path.relative_to(dst))

patch(F / "Core/Inc/main.h", [
    ("#define USER_BUTTON_Pin GPIO_PIN_12\n#define USER_BUTTON_GPIO_Port GPIOD\n#define USER_BUTTON_EXTI_IRQn EXTI12_IRQn",
     "#define USER_BUTTON_Pin GPIO_PIN_7\n#define USER_BUTTON_GPIO_Port GPIOE\n#define USER_BUTTON_EXTI_IRQn EXTI7_IRQn"),
    ("#define BOOT_Pin GPIO_PIN_5\n#define BOOT_GPIO_Port GPIOD",
     "#define BOOT_Pin GPIO_PIN_13\n#define BOOT_GPIO_Port GPIOE"),
    ("#define SPI_MISO_Pin GPIO_PIN_1\n#define SPI_MISO_GPIO_Port GPIOG",
     "#define SPI_MISO_Pin GPIO_PIN_8\n#define SPI_MISO_GPIO_Port GPIOH"),
])

patch(F / "Core/Src/stm32n6xx_hal_msp.c", [
    ("    __HAL_RCC_GPIOG_CLK_ENABLE();\n    /**SPI5 GPIO Configuration",
     "    __HAL_RCC_GPIOG_CLK_ENABLE();\n    __HAL_RCC_GPIOH_CLK_ENABLE();\n    /**SPI5 GPIO Configuration"),
    ("    GPIO_InitStruct.Pin = SPI_MISO_Pin|SPI_MOSI_Pin;\n    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;\n"
     "    GPIO_InitStruct.Pull = GPIO_NOPULL;\n    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;\n"
     "    GPIO_InitStruct.Alternate = GPIO_AF5_SPI5;\n    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);",
     "    GPIO_InitStruct.Pin = SPI_MOSI_Pin;\n    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;\n"
     "    GPIO_InitStruct.Pull = GPIO_NOPULL;\n    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;\n"
     "    GPIO_InitStruct.Alternate = GPIO_AF5_SPI5;\n    HAL_GPIO_Init(SPI_MOSI_GPIO_Port, &GPIO_InitStruct);\n\n"
     "    /* DK : MISO sur PH8 (D12). Vérifier l'AF de PH8 dans la datasheet / CubeMX (AF5 attendu). */\n"
     "    GPIO_InitStruct.Pin = SPI_MISO_Pin;\n"
     "    HAL_GPIO_Init(SPI_MISO_GPIO_Port, &GPIO_InitStruct);"),
    ("    HAL_GPIO_DeInit(GPIOG, SPI_MISO_Pin|SPI_MOSI_Pin);",
     "    HAL_GPIO_DeInit(SPI_MOSI_GPIO_Port, SPI_MOSI_Pin);\n    HAL_GPIO_DeInit(SPI_MISO_GPIO_Port, SPI_MISO_Pin);"),
])

patch(F / "Core/Src/main.c", [
    ("  __HAL_RCC_GPIOA_CLK_ENABLE();\n\n  /*Configure GPIO pin Output Level */\n  HAL_GPIO_WritePin(CHIP_EN_GPIO_Port",
     "  __HAL_RCC_GPIOA_CLK_ENABLE();\n  __HAL_RCC_GPIOH_CLK_ENABLE();\n\n  /*Configure GPIO pin Output Level */\n  HAL_GPIO_WritePin(CHIP_EN_GPIO_Port"),
    ("HAL_EXTI_ConfigLineAttributes(EXTI_LINE_12, EXTI_LINE_SEC);",
     "HAL_EXTI_ConfigLineAttributes(EXTI_LINE_7, EXTI_LINE_SEC);"),
])

it = F / "Core/Src/stm32n6xx_it.c"
t = it.read_text()
t2 = re.sub(r"EXTI12_IRQ", "EXTI7_IRQ", t)
if t2 == t:
    sys.exit("EXTI12 introuvable dans stm32n6xx_it.c")
it.write_text(t2)
print("modifié :", it.relative_to(dst))
for h in (F / "Core/Inc").glob("stm32n6xx_it.h"):
    h.write_text(h.read_text().replace("EXTI12_IRQHandler", "EXTI7_IRQHandler"))
    print("modifié :", h.relative_to(dst))

print(f"\nProjet DK prêt : {dst}\n"
      "Dans STM32CubeIDE : File > Import > Existing Projects, dossier STM32CubeIDE du projet _DK.\n"
      "Loader externe à utiliser pour flasher : MX66UW1G45G_STM32N6570-DK.stldr")
