# Linden Shore Assessment
Linden Shore Coding Assessment - PnL Calculaton. This project implements a Profit and Loss (PnL) calculator for trading analysis using FIFO and LIFO accounting methods.

### Compilation
```bash
g++ -o pnl_calculator_main pnl_calculator_main.cpp
```

### Usage
```bash
./pnl_calculator_main <csv_file> <fifo|lifo>
```

### Examples
```bash
# FIFO accounting
./pnl_calculator_main data/test_trades.csv fifo

# LIFO accounting  
./pnl_calculator_main data/test_trades.csv lifo

# Multi-symbol test
./pnl_calculator_main data/multi_symbol_trades.csv fifo
./pnl_calculator_main data/multi_symbol_trades.csv lifo
```
