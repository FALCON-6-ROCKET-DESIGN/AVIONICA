# 🤝 Guia de Contribuição e Uso do Git - Setor de Aviônica

Bem-vindo(a) ao repositório oficial do Setor de Aviônica da **FALCON-6 Rocket Design**!

Para manter o código estável, evitar conflitos e garantir que a versão principal (`main`) esteja sempre funcional para testes e lançamentos, **nenhum colaborador deve fazer alterações diretamente na branch `main`**. Todo trabalho deve ser feito em uma **branch individual com o seu nome**.

---

## 1. Configuração Inicial (Primeira vez no computador)

Se você ainda não baixou o projeto ou nunca configurou o Git nesta máquina, abra o terminal na pasta onde deseja salvar o projeto e execute:

1. **Clonar o repositório:**
   ```bash
   git clone [https://github.com/FALCON-6-ROCKET-DESIGN/AVIONICA.git](https://github.com/FALCON-6-ROCKET-DESIGN/AVIONICA.git)
   cd AVIONICA
   ```

2. **Configurar seu nome, e-mail e usuário GitHub apenas para esta pasta (`--local`):**
   *(Usar `--local` e vincular seu usuário na URL evita conflitos em computadores compartilhados do laboratório).*
   ```bash
   git config --local user.name "Seu Nome Completo"
   git config --local user.email "seu.email@exemplo.com"
   git remote set-url origin https://SEU-USUARIO-GITHUB@github.com/FALCON-6-ROCKET-DESIGN/AVIONICA.git
   ```

3. **Conferir se a configuração foi aplicada:**
   ```bash
   git config --local --list
   ```

4. **Criar a sua branch individual** (apenas se você ainda **não** tiver uma branch no GitHub):
   ```bash
   git checkout -b seu-nome
   ```
   *(Exemplo: `git checkout -b ricardo-rubio` — se você já tem uma branch criada, pule para a Seção 2).*

---

## 2. Entrando em uma Nova Máquina (Puxando sua Branch já existente)

Se você já criou sua branch anteriormente em outro PC e quer continuar de onde parou em um computador novo, **sem** sobrescrever seus arquivos com o que está na `main`:

1. Após clonar e configurar seu usuário (passos 1 e 2 da Seção 1), baixe as informações das branches do GitHub:
   ```bash
   git fetch origin
   ```

2. Entre na sua branch existente puxando exatamente o conteúdo dela do GitHub:
   ```bash
   git checkout -b seu-nome origin/seu-nome
   ```
   *(Nota: Se a pasta já foi clonada antes nesse PC e você só quer entrar e atualizar sua branch sem tocar na `main`, use `git checkout seu-nome` seguido de `git pull origin seu-nome`).*

---

## 3. Salvando e Enviando Alterações para a Sua Branch

Sempre que criar ou modificar códigos, esquemáticos ou modelos 3D:

1. **Confira em qual branch você está:**
   ```bash
   git branch
   ```
   *(O asterisco `*` verde deve estar ao lado do seu nome, nunca em `main`).*

2. **Adicione os arquivos modificados e registre o commit:**
   ```bash
   git add .
   git commit -m "Descreva brevemente o que você fez"
   ```

3. **Envie as alterações para a sua branch no GitHub:**
   ```bash
   git push -u origin seu-nome
   ```
   *(Nos envios seguintes, basta digitar `git push`).*

---

## 4. Enviando suas Atualizações para a `main` (Pull Request)

Quando o seu código ou arquivo estiver testado e pronto para entrar na versão oficial da equipe:

1. Acesse o repositório **AVIONICA** no GitHub pelo navegador.
2. Clique no botão verde **"Compare & pull request"** no topo da página (ou vá em **Pull requests** > **New pull request**).
3. Verifique se o destino está como **`base: main`** e a origem como **`compare: seu-nome`**.
4. Escreva um título e uma descrição clara do que foi alterado e clique em **Create pull request**.
5. Aguarde a revisão e aprovação da gerência de aviônica para que a alteração seja integrada (`Merge`) à `main`.

---

## 5. Puxando Atualizações da `main` (Sobrescrevendo arquivos antigos)

Se outros membros atualizaram a `main` depois de você (por exemplo: você criou a primeira versão do código do cartão SD, e dias depois outro colaborador atualizou esse mesmo arquivo na `main`), siga uma das opções abaixo para trazer a versão mais recente para a sua branch:

### Opção A: Puxar novidades da `main` dando prioridade aos arquivos oficiais
Este comando baixa o que há de novo na `main` e, caso exista algum arquivo modificado tanto por você quanto na `main`, sobrescreve automaticamente pela versão mais recente da `main`:

```bash
git fetch origin
git merge origin/main -X theirs
git push origin seu-nome
```

### Opção B: Resetar sua branch para ficar 100% igual à `main` atual
Se você quer descartar todas as suas modificações locais antigas e deixar sua pasta exatamente igual à `main` oficial do GitHub:

```bash
git fetch origin
git reset --hard origin/main
git push --force origin seu-nome
```
*(Atenção: O `reset --hard` apaga alterações locais que ainda não foram enviadas para a `main`).*

---

## 📌 Boas Práticas

- **Mensagens de commit claras:** Evite `"teste"` ou `"atualização"`. Prefira `"Adiciona leitura do sensor BMP280"` ou `"Corrige pinagem da placa de célula de carga"`.
- **Respeite a organização:** Coloque cada arquivo na sua respectiva pasta (`firmware/`, `hardware/`, `mecanica_3d/`, `software_solo/`, `docs/`).
- **Dúvidas:** Caso ocorra algum erro ou conflito no Git, procure a gerência do setor antes de forçar comandos.
