# 🚀 Estação de Solo — Telemetria LoRa E22 + ESP32

Sistema de recepção, visualização em tempo real e análise pós-voo para aviônica de foguetemodelismo baseada em **ESP32**, **LoRa Ebyte E22-900T30D**, **BMP280**, **BNO055**, **GPS NEO-6M** e **Cartão Micro SD**.

---

## 📁 Estrutura de Arquivos da Estação de Solo

Coloque todos os arquivos abaixo dentro da **mesma pasta** no computador:

```text
Estacao_de_Solo/
├── Dashboard.py                   # Interface principal (Streamlit)
├── Simulador.py                   # Gerador de voo simulado para testes sem hardware
├── Falcon6_Mapa_UNIFEI.mbtiles    # Arquivo de limites geográficos da área de lançamento
├── requirements.txt               # Lista de bibliotecas Python necessárias
└── README.md                      # Este guia de instalação e operação