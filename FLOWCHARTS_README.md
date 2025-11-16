# Project Flowcharts - USB to Amiga Keyboard Adapter

Questo documento descrive i diagrammi di flusso del progetto **USB to Amiga Keyboard Adapter** basato su FreeRTOS.

## 📊 Diagrammi Disponibili

### 1. **Project Flowchart** (`project_flowchart.svg/png`)
**Panoramica generale dell'architettura del progetto**

- **Scopo**: Mostra la struttura generale del sistema con tutti i componenti principali
- **Livelli mostrati**:
  - Hardware Layer (USB Keyboard, STM32F401, Amiga Computer)
  - FreeRTOS Kernel (Scheduler, Code)
  - Application Tasks (USB Task, Amiga Task)
  - Hardware Abstraction Layer (Driver STM32)
  - System Initialization (main, init)

### 2. **Detailed Architecture** (`detailed_architecture.svg/png`)
**Architettura dettagliata con flusso dati**

- **Scopo**: Mostra il flusso dettagliato dei dati tra i componenti
- **Dettagli inclusi**:
  - Strutture dati (keyboard_message_t, led_message_t)
  - Timing dei task (10ms per tutti i task principali)
  - Priorità dei task (USB=Normal, Amiga=Low, LedManager=Low)
  - Dettagli del protocollo Amiga
  - Dimensioni delle code FreeRTOS

### 3. **Timing Flow** (`timing_flow.svg/png`)
**Flusso temporale delle operazioni**

- **Scopo**: Mostra la sequenza temporale delle operazioni del sistema
- **Sezioni**:
  - Sequenza di startup del sistema
  - Cicli runtime dei task
  - Timing del protocollo Amiga (livello microsecondo)
  - Gestione degli errori
  - Annotazioni temporali precise

## 🎯 **Architettura FreeRTOS**

### **Task Principali**

| Task | Priorità | Ciclo | Stack | Responsabilità |
|------|----------|-------|-------|----------------|
| **USB Task** | Normal | 10ms | 1024 | USB HID, LED, Stato connessione |
| **Amiga Task** | Low | 10ms | 1024 | GPIO, Protocollo Amiga, Reset |
| **LedManagerTask** | Low | Event-Driven | 512 | Gestione LED di stato onboard |

### **Comunicazione Inter-Task**

| Coda | Dimensione | Direzione | Contenuto |
|------|------------|-----------|-----------|
| **keyboardQueue** | 16 messaggi | USB → Amiga | keyboard_message_t |
| **ledQueue** | 16 messaggi | Amiga → USB | led_message_t |
| **ledManagerQueue**| 32 messaggi | USB → LedManager| led_manager_message_t |
| **amigaTaskQueue** | 8 messaggi | All → Amiga | AmigaTaskMsg_t |


### **Timing Critici**

| Operazione | Timing | Descrizione |
|------------|--------|-------------|
| **USB Task Cycle** | 10ms | Ciclo principale USB task |
| **Amiga Task Cycle** | 10ms | Ciclo principale Amiga task |
| **Clock Pulse** | 20μs | Durata impulso clock Amiga |
| **Handshake** | 85μs | Impulso handshake CPU Amiga |
| **Reset Detection** | 500ms | Timeout per rilevamento reset |

## 🔧 **Protocollo Amiga**

### **Caratteristiche Principali**
- **Ordine bit**: 6-5-4-3-2-1-0-7 (bit 7 = flag up/down)
- **Logica invertita**: 0 = HIGH (5V), 1 = LOW (0V)
- **Velocità**: ~17 kbits/sec (60μs per bit)
- **Handshake**: Obbligatorio impulso 85μs da CPU

### **Sequenza Trasmissione**
1. Setup data line (20μs)
2. Clock pulse LOW (20μs)
3. Clock pulse HIGH (20μs)
4. Ripeti per 8 bit
5. Attendi handshake CPU (85μs)

## 🚀 **Flusso di Esecuzione**

### **Startup Sequence**
1. `HAL_Init()` - Inizializzazione HAL
2. `SystemClock_Config()` - Configurazione clock
3. `MX_GPIO_Init()` - Inizializzazione GPIO
4. `MX_USART2_UART_Init()` - Inizializzazione UART debug
5. `create_tasks_and_queues()` - Creazione task e code
6. `vTaskStartScheduler()` - Avvio scheduler FreeRTOS

### **Runtime Operation**
- **USB Task**: Gestisce USB HID e LED ogni 10ms
- **Amiga Task**: Processa tastiera e gestisce reset ogni 10ms
- **Comunicazione**: Via code FreeRTOS thread-safe

## 📁 **File Generati**

```
project_flowchart.dot       # Sorgente Graphviz - Panoramica generale
project_flowchart.svg       # Diagramma SVG - Panoramica generale  
project_flowchart.png       # Diagramma PNG - Panoramica generale

detailed_architecture.dot   # Sorgente Graphviz - Architettura dettagliata
detailed_architecture.svg   # Diagramma SVG - Architettura dettagliata
detailed_architecture.png   # Diagramma PNG - Architettura dettagliata

timing_flow.dot             # Sorgente Graphviz - Flusso temporale
timing_flow.svg             # Diagramma SVG - Flusso temporale
timing_flow.png             # Diagramma PNG - Flusso temporale
```

## 🛠️ **Rigenerazione Diagrammi**

Per rigenerare i diagrammi:

```bash
# SVG (vettoriale)
dot -Tsvg project_flowchart.dot -o project_flowchart.svg
dot -Tsvg detailed_architecture.dot -o detailed_architecture.svg  
dot -Tsvg timing_flow.dot -o timing_flow.svg

# PNG (alta risoluzione)
dot -Tpng project_flowchart.dot -o project_flowchart.png -Gdpi=300
dot -Tpng detailed_architecture.dot -o detailed_architecture.png -Gdpi=300
dot -Tpng timing_flow.dot -o timing_flow.png -Gdpi=300
```

## 📋 **Note**

- I diagrammi sono ottimizzati per la documentazione tecnica
- I colori identificano i diversi livelli architetturali
- Le frecce tratteggiate indicano connessioni hardware
- Le frecce punteggiate indicano timing/cicli
- I diagrammi sono scalabili (formato SVG disponibile)

---
**Generato automaticamente per il progetto USB to Amiga Keyboard Adapter v3.0NG-RTOS**