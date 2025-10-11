# FreeRTOS Project Fixes

This README documents the fixes applied to a FreeRTOS project generated with STM32CubeMX, addressing compilation errors and task scheduling issues.

## Problem Description

The project initially presented two main problems:
1.  **Compilation Error:** Duplicate definitions of `PendSV_Handler` and `SVC_Handler` leading to linker errors.
2.  **Runtime Issue:** Although tasks were launched (evidenced by `printf` outputs), the FreeRTOS scheduler was not effectively switching between them, resulting in a stalled execution.

## Solutions Implemented

### 1. Resolution of Duplicate Interrupt Handlers (`PendSV_Handler`, `SVC_Handler`)

**Diagnosis:**
STM32CubeMX generates default implementations for `PendSV_Handler` and `SVC_Handler` in `Src/stm32f4xx_it.c`. When FreeRTOS is integrated, it provides its own, more robust implementations of these handlers, which are essential for context switching. The presence of both sets of definitions leads to linker errors due to multiple definitions of the same symbols.

**Solution:**
The conflicting definitions in `Src/stm32f4xx_it.c` were disabled using preprocessor directives (`#if 0` and `#endif`). This prevents the compiler from processing the CubeMX-generated empty handlers, allowing the FreeRTOS-provided handlers to be used without conflict.

**Changes in `Src/stm32f4xx_it.c`:**

**For `SVC_Handler`:**
```diff
--- a/Src/stm32f4xx_it.c
+++ b/Src/stm32f4xx_it.c
@@ -137,14 +137,16 @@
 /**
 * @brief This function handles System service call via SWI instruction.
 */
-void SVC_Handler(void)
-{
-  /* USER CODE BEGIN SVCall_IRQn 0 */
-
-  /* USER CODE END SVCall_IRQn 0 */
-  /* USER CODE BEGIN SVCall_IRQn 1 */
-
-  /* USER CODE END SVCall_IRQn 1 */
-}
+#if 0
+void SVC_Handler(void)
+{
+  /* USER CODE BEGIN SVCall_IRQn 0 */
+
+  /* USER CODE END SVCall_IRQn 0 */
+  /* USER CODE BEGIN SVCall_IRQn 1 */
+
+  /* USER CODE END SVCall_IRQn 1 */
+}
+#endif
 
 /**
 * @brief This function handles Debug monitor.
```

**For `PendSV_Handler`:**
```diff
--- a/Src/stm32f4xx_it.c
+++ b/Src/stm32f4xx_it.c
@@ -165,14 +165,16 @@
 /**
 * @brief This function handles Pendable request for system service.
 */
-void PendSV_Handler(void)
-{
-  /* USER CODE BEGIN PendSV_IRQn 0 */
-
-  /* USER CODE END PendSV_IRQn 0 */
-  /* USER CODE BEGIN PendSV_IRQn 1 */
-
-  /* USER CODE END PendSV_IRQn 1 */
-}
+#if 0
+void PendSV_Handler(void)
+{
+  /* USER CODE BEGIN PendSV_IRQn 0 */
+
+  /* USER CODE END PendSV_IRQn 0 */
+  /* USER CODE BEGIN PendSV_IRQn 1 */
+
+  /* USER CODE END PendSV_IRQn 1 */
+}
+#endif
 
 /**
 * @brief This function handles System tick timer.
```

### 2. Resolution of Task Scheduling Issues (SysTick Configuration)

**Diagnosis:**
The FreeRTOS scheduler relies heavily on the SysTick interrupt to manage task switching. Incorrect configuration of the SysTick interrupt, particularly its priority or the absence of a call to the FreeRTOS tick handler, prevents the scheduler from functioning correctly.

Two specific issues were identified:
1.  **SysTick Interrupt Priority:** The SysTick interrupt was configured with the highest priority (0) in `Src/main.c`. For FreeRTOS, the SysTick interrupt must have the *lowest* possible priority to ensure that FreeRTOS API calls can be made safely from other interrupts without being preempted by the scheduler itself.
2.  **Missing FreeRTOS Tick Handler Call:** The `SysTick_Handler` in `Src/stm32f4xx_it.c` was calling `HAL_SYSTICK_IRQHandler()`, which in turn called a weak, empty `HAL_SYSTICK_Callback()`. This meant the FreeRTOS kernel was not being notified of the SysTick events, thus failing to perform task scheduling.

**Solution:**
1.  **Correct SysTick Priority:** The SysTick interrupt priority was adjusted to the lowest possible value (15) in `Src/main.c`.
2.  **Integrate FreeRTOS Tick Handler:** The `SysTick_Handler` in `Src/stm32f4xx_it.c` was modified to directly call `HAL_IncTick()` (for HAL timekeeping) and `osSystickHandler()` (the FreeRTOS kernel tick handler).

**Changes in `Src/main.c`:**

```diff
--- a/Src/main.c
+++ b/Src/main.c
@@ -249,7 +249,7 @@
   HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);
 
   /* SysTick_IRQn interrupt configuration */
-  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
+  HAL_NVIC_SetPriority(SysTick_IRQn, 15, 0);
 }
 
 /* USART2 init function */
```

**Changes in `Src/stm32f4xx_it.c`:**

```diff
--- a/Src/stm32f4xx_it.c
+++ b/Src/stm32f4xx_it.c
@@ -176,9 +176,10 @@
   /* USER CODE END PendSV_IRQn 1 */
 }
 #endif
-
 /**
 * @brief This function handles System tick timer.
 */
 void SysTick_Handler(void)
 {
   /* USER CODE BEGIN SysTick_IRQn 0 */
 
   /* USER CODE END SysTick_IRQn 0 */
-  HAL_SYSTICK_IRQHandler();
+  HAL_IncTick();
+  osSystickHandler();
   /* USER CODE BEGIN SysTick_IRQn 1 */
 
   /* USER CODE END SysTick_IRQn 1 */
```

## How to Compile the Project

To compile the project after these fixes, navigate to the project's root directory in your terminal and execute the following commands:

1.  **Clean the project:**
    ```bash
    make clean
    ```
2.  **Build the project:**
    ```bash
    make
    ```

These steps will ensure that the project is rebuilt with the applied fixes, resolving the previously encountered compilation and runtime issues.
