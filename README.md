# 🚀 Setor de Aviônica - FALCON-6 Rocket Design

Repositório central de desenvolvimento de hardware, software embarcado, telemetria e sistemas de solo do setor de Aviônica da equipe **FALCON-6 Rocket Design** (UNIFEI - Campus Itabira).

---

## 📂 Estrutura do Repositório

- **`firmware/`**: Códigos-fonte para os microcontroladores (sistemas embarcados de bordo, receptores de solo e célula de carga) + tutorial de configuração da Arduino IDE.
  - `transmissor_foguete/`: Código embarcado na eletrônica do foguete.
  - `receptor_base/`: Código do receptor na base (LoRa / Ground Station).
  - `celula_carga/`: Código de aquisição e leitura da célula de carga (teste de motor).
- **`hardware/`**: Arquivos de projeto de Circuito Impresso (PCBs), esquemáticos e Gerber (desenvolvidos em KiCad / EasyEDA).
  - `placa_telemetria_tx/`: PCB e esquemas do transmissor (foguete).
  - `placa_receptor_rx/`: PCB e esquemas do receptor (base).
  - `placa_celula_carga/`: PCB e esquemas da placa para teste de motor.
- **`mecanica_3d/`**: Modelos 3D (`.step`, `.stl`) das *cases* de proteção.
  - `case_foguete/`: Case interna do transmissor/aviônica no foguete.
  - `case_base/`: Case do receptor na base.
  - `case_celula_carga/`: Case da placa de aquisição da célula de carga.
- **`software_solo/`**: Aplicação de *Ground Station* (Dashboard) para monitoramento em tempo real dos dados de voo.
- **`docs/`**: Documentação técnica, esquemas de ligação, pinouts e folhas de dados (*datasheets*).

---

## 🛠️ Tecnologias e Ferramentas Utilizadas

- **Microcontroladores & Comunicação:** ESP32, Módulos LoRa (E220 / E22).
- **Sensores:** Barômetros, IMUs, GPS e amplificadores para célula de carga.
- **Desenvolvimento de Hardware:** KiCad / EasyEDA Pro.
- **Software de Solo:** Python (Streamlit / bibliotecas de gráficos e mapas offline).
- **Impressão 3D:** Modelagem mecânica para acomodação segura das placas nas *cases*.

---

## 🤝 Como Contribuir

Antes de enviar qualquer arquivo ou alteração, leia obrigatoriamente o **[Guia de Contribuição (`CONTRIBUTING.md`)](CONTRIBUTING.md)** para configurar o Git e criar a sua branch individual.

---

## 👥 Equipe e Contato

- **Instituição:** Universidade Federal de Itajubá (UNIFEI) - Campus Itabira
- **Equipe:** FALCON-6 Rocket Design
- **Gerência de Aviônica:** Ricardo Ruiz Rubio