# Контроллер PLL LMX2592R

Прошивка контроллера синтезатора частоты **TI LMX2592R** на базе **STM32F051** (Cortex-M0+) под **FreeRTOS**. Управление регистрами PLL по SPI, взаимодействие с оператором — через командную строку (FreeRTOS+CLI) поверх UART.

## Аппаратная часть

| Назначение                | Порт/вывод | Периферия      |
|---------------------------|------------|----------------|
| LMX SPI SCK               | PA5        | SPI1           |
| LMX SPI MISO (SDO)        | PA6        | SPI1           |
| LMX SPI MOSI (SDI)        | PA7        | SPI1           |
| LMX Chip Select           | PA3        | GPIO (софт CS) |
| LMX Enable (Low=Power down, High=Active) | PA4 | GPIO |
| LMX Lock Detect (вход)    | PA6        | GPIO           |
| UART TX (USART1)          | PB6        | USART1         |
| UART RX (USART1)          | PB7        | USART1         |

Назначения выводов — в [Core/inc/hardware.h](Core/inc/hardware.h).

UART: **9600 8N1**. Рекомендуемый терминал — [Termite](https://www.compuphase.com/software_termite.htm) (хранит историю введённых команд).

## CLI-команды

Прошивка поднимает интерпретатор командной строки FreeRTOS+CLI прямо на UART. После загрузки в терминале появляется приглашение `>`.

| Команда                     | Описание |
|------------------------------|----------|
| `echo <text>`                 | Эхо введённого текста — проверка канала связи |
| `wr <register> <value>`       | Запись значения в регистр LMX по SPI. Регистр можно указывать как `R43`, так и просто `43`. Значение — hex (`0x2B0000`) или десятичное. Формат соответствует экспорту из **TICS Pro** (старший байт — номер регистра, младшие два — данные) |
| `rr <register>`                | Чтение регистра LMX по SPI (`rr R43` или `rr 43`) |
| `enable`                       | Включить LMX (`LMX_ENABLE_PIN` в high) |
| `disable`                      | Выключить LMX / power down (`LMX_ENABLE_PIN` в low) |
| `reset`                        | Программный сброс LMX power-cycle'ом `LMX_ENABLE_PIN` |
| `lock`                         | Прочитать состояние вывода Lock Detect (`LOCKED` / `UNLOCKED`) |

Примеры:
```
> wr R43 0x2B0000
R43 = 0x2B0000
> rr 43
R43 = 0x08CF
> enable
LMX enabled
> lock
LockDetect: LOCKED
```

## Инструменты (Tools/)

- **[HexRegisterValues.txt](Tools/HexRegisterValues.txt)** — пример дампа регистров, экспортированного из TICS Pro (`R<n><TAB>0x<value>` построчно, в порядке программирования).
- **[WriteRegisters.ps1](Tools/WriteRegisters.ps1)** — PowerShell-скрипт, который построчно прогоняет такой файл через `wr` по UART, затем включает LMX (`enable`) и опрашивает `lock` до 5 секунд в ожидании захвата частоты:
  ```powershell
  .\Tools\WriteRegisters.ps1 -Port COM5
  ```

## Сборка

CMake + Ninja, тулчейн `arm-none-eabi-gcc` (см. [cmake/toolchain-arm-none-eabi.cmake](cmake/toolchain-arm-none-eabi.cmake)).

```powershell
cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

Логирование (RTT, уровень выставляется через `CONFIG_LOG_MAXIMUM_LEVEL`) — через SEGGER RTT / J-Link.

## Программный стек

- FreeRTOS (статическое выделение памяти)
- FreeRTOS+CLI — командный интерпретатор
- SEGGER RTT — отладочный лог
- STM32 LL-драйверы (без HAL, кроме служебной инициализации)
