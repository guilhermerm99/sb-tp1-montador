# Trabalho Prático 1 – Software Básico (UnB)

**Disciplina:** Software Básico  
**Professor:** Bruno Macchiavello  

---

## 👥 Alunos
| Nome | Matrícula |
|------|-----------|
| Guilherme Ribeiro de Macedo | 170162354 |
| Giulia Moura Ferreira | 200018795 |

---

## 💻 Sistema operacional e ambiente
- Windows 11 + WSL Ubuntu 22.04  
- Compilador: `g++` (GCC)  
- Padrão C++: `C++17`  

---

## 🛠️ Como compilar
```bash
g++ -std=c++17 -o montador montador.cpp
```

## ▶️ Como executar
O programa possui três modos de operação, definidos pela extensão do arquivo de entrada.

### 1. Pré‑processamento
```bash
./montador exemplo.asm
```
- Gera o arquivo `exemplo.pre`
- Remove comentários, normaliza maiúsculas/minúsculas, resolve as diretivas `EQU` e `IF`
- Reordena as seções (`SECTION TEXT` primeiro, `SECTION DATA` por último)

### 2. Montagem
```bash
./montador exemplo.pre
```
- Gera `exemplo.obj` (código objeto) e `exemplo.pen`
- O `.obj` contém apenas os números separados por espaço
- O `.pen` contém o mesmo código, com `-1` nos locais que tiveram pendências

### 3. Simulação
```bash
./montador exemplo.obj
```
- Executa o programa em um simulador da arquitetura
- Suporta todas as instruções da disciplina (`ADD`, `SUB`, `MULT`, `DIV`, `JMP`, `JMPN`, `JMPP`, `JZ`, `COPY`, `LOAD`, `STORE`, `INPUT`, `OUTPUT`, `STOP`)
- Protege contra divisão por zero e acessos inválidos à memória

---

## 📌 Observações
- Montador implementado em **duas passagens**, com *backpatching* para referências antecipadas (*forward references*).
- Suporte completo às diretivas `CONST`, `SPACE`, `EQU` e `IF`.
- Validação rígida de seções (`SECTION TEXT` só aceita instruções; `SECTION DATA` só aceita `CONST` e `SPACE`).
- Rótulos podem ser separados da instrução por quebra de linha; comentários iniciados por `;` são ignorados.
- **Case insensitive**: maiúsculas e minúsculas são tratadas igualmente.
- O comando `COPY` exige os operandos separados por vírgula **sem espaço** (ex.: `COPY A,B`).
- Nenhuma biblioteca externa foi utilizada – apenas a biblioteca padrão C++.