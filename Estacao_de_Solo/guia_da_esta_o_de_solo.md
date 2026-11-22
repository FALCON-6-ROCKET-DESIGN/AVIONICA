# 🚀 Estação de Solo — Telemetria LoRa E22 + ESP32

Guia completo de instalação, configuração e operação do sistema de recepção em tempo real (`Dashboard.py`), simulador de voo (`Simulador.py`) e análise pós-voo do Cartão Micro SD.

---

## 📁 Estrutura Recomendada da Pasta

Mantenha todos os arquivos abaixo dentro da **mesma pasta** no computador:

```text
Estacao_de_Solo/
├── Dashboard.py                   # Interface principal (Streamlit)
├── Simulador.py                   # Gerador de voo simulado para testes sem hardware
├── Falcon6_Mapa_UNIFEI.mbtiles    # Arquivo de limites geográficos da área de lançamento
├── requirements.txt               # Lista de bibliotecas Python necessárias
└── README.md                      # Este guia de instalação e operação
```

*(Nota: O arquivo `telemetria_guardada.csv` será criado automaticamente nesta pasta assim que os primeiros pacotes forem recebidos).*

---

## 🛠️ 1. Pré-Requisitos na Máquina

### A. Python (Versão 3.9 ou superior)
1. Baixe o instalador oficial em [python.org/downloads](https://www.python.org/downloads/).
2. **Importante (Windows):** Na primeira tela da instalação, marque a caixa **"Add python.exe to PATH"** antes de clicar em *Install Now*.
3. Para conferir se deu certo, abra o terminal (CMD ou PowerShell) e digite:
   ```bash
   python --version
   ```

### B. Driver USB-Serial do ESP32 (Para operação com hardware)
Para que o Windows/Linux reconheça o ESP32 Receptor na porta COM:
* **ESP32 com chip CP2102 (quadrado):** Instale o *CP210x Universal Windows Driver* (Silicon Labs).
* **ESP32 com chip CH340 (retangular):** Instale o driver *CH341SER* (WCH).

---

## 📦 2. Instalação das Bibliotecas Python

Abra o terminal dentro da pasta `Estacao_de_Solo` e execute o comando abaixo para instalar todas as dependências externas (caso tenha o arquivo `requirements.txt`):

```bash
pip install -r requirements.txt
```

Ou instale diretamente pelo comando manual:

```bash
pip install streamlit pyserial pandas numpy folium streamlit-folium plotly
```

### Resumo das Bibliotecas Utilizadas
* **Externas (instaladas pelo comando acima):** `streamlit`, `pyserial`, `pandas`, `numpy`, `folium`, `streamlit-folium`, `plotly`.
* **Nativas (já vêm no Python):** `struct`, `sqlite3`, `time`, `datetime`, `os`, `math`, `random`.

---

## ▶️ 3. Como Rodar o Sistema

### Modo A: Teste Offline na Bancada (Sem ESP32 / Usando o Simulador)
Ideal para validar se todas as bibliotecas e gráficos estão funcionando na máquina:

1. Abra um **Terminal 1** na pasta do projeto e inicie o Dashboard:
   ```bash
   streamlit run Dashboard.py
   ```
   *(Se o comando `streamlit` não for reconhecido no Windows, use: `python -m streamlit run Dashboard.py`)*
2. O navegador abrirá automaticamente em `http://localhost:8501`.
3. Na barra lateral esquerda, mantenha **"🔄 Atualização em Tempo Real"** ativado e deixe **"Conectar ao ESP32"** desmarcado.
4. Abra um **Terminal 2** na mesma pasta e execute o simulador:
   ```bash
   python Simulador.py
   ```
5. Acompanhe a trajetória 3D, o mapa e os gráficos atualizando ao vivo. Pressione `Ctrl + C` no Terminal 2 para parar o simulador.

### Modo B: Operação Real com ESP32 Receptor (LoRa E22)
1. **Atenção à Antena:** Certifique-se de que a antena 915 MHz esteja rosqueada no módulo LoRa E22 antes de conectar o USB.
2. **Feche a Arduino IDE (Monitor Serial):** Apenas um programa pode acessar a porta COM por vez. Se o Monitor Serial estiver aberto, o Python exibirá erro de acesso negado (`PermissionError`).
3. Inicie o Dashboard no terminal:
   ```bash
   streamlit run Dashboard.py
   ```
4. Na barra lateral esquerda do Dashboard:
   * Selecione a **Porta COM** correspondente ao ESP32 Receptor (ex: `COM3`, `COM4` ou `/dev/ttyUSB0`).
   * Deixe o **Baud Rate** em `115200`.
   * Marque a caixa **"Conectar ao ESP32"**.
5. Assim que o ESP32 Transmissor (aviônica) enviar pacotes, o contador **ID Pacote** subirá e os gráficos serão plotados em tempo real.

---

## 🗂️ 4. Análise Pós-Voo (Cartão Micro SD)

Durante o voo, o LoRa transmite um pacote resumido de **55 bytes** para manter o link rápido e confiável, enquanto o Cartão SD embarcado grava **47 colunas completas a 10 Hz** no arquivo `voo_telemetria.csv`.

Após recuperar o foguete:
1. Retire o cartão Micro SD da aviônica e insira no computador.
2. Role o `Dashboard.py` até a última seção: **"🗂️ Análise Pós-Voo (dados completos do Cartão SD)"**.
3. Faça o upload do arquivo `voo_telemetria.csv` para visualizar os gráficos completos de Giroscópio, Magnetômetro, Aceleração Linear e Orientação em alta resolução.

---

## 🔧 5. Solução Rápida de Problemas

| Sintoma | Causa Provável | Solução |
| :--- | :--- | :--- |
| `Erro ao conectar: PermissionError / Access is denied` | O Monitor Serial da Arduino IDE está aberto na mesma porta COM. | Feche o Monitor Serial (ou a Arduino IDE) e marque *"Conectar ao ESP32"* novamente. |
| `Nenhuma porta encontrada` | Cabo USB apenas de carga (sem fios de dados) ou driver ausente. | Troque o cabo USB ou instale o driver CP2102 / CH340. |
| Conectado ao ESP32, mas `ID Pacote` continua em `0` | Transmissor desligado, fios `TXD/RXD` invertidos ou pinos `M0/M1` fora do `GND`. | Verifique se `M0` e `M1` estão aterrados no `GND` e se `TXD` do LoRa está no `GPIO 33`. |
| Gráficos antigos aparecem antes do novo lançamento | Dados de um teste anterior ficaram salvos em `telemetria_guardada.csv`. | Clique no botão **"🗑️ Limpar Histórico Salvo (LoRa)"** no final da página antes do lançamento. |