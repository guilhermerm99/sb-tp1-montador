# Trabalho Prático 1 – Software Básico (UnB)
## Montador e Simulador de Assembly

**Disciplina:** Software Básico  
**Professor:** Bruno Macchiavello  
**Semestre:** 2026/1

---

## Equipe

| Nome | Matrícula |
|------|-----------|
| Guilherme Ribeiro de Macedo | 170162354 |
| Giulia Moura Ferreira | 200018795 |

---

## Índice

1. [Visão Geral](#visão-geral)
2. [Requisitos do Sistema](#requisitos-do-sistema)
3. [Compilação](#compilação)
4. [Modos de Operação](#modos-de-operação)
5. [Arquitetura do Montador](#arquitetura-do-montador)
6. [Especificação Técnica](#especificação-técnica)
7. [Exemplos de Uso](#exemplos-de-uso)
8. [Tratamento de Erros](#tratamento-de-erros)
9. [Estrutura do Código](#estrutura-do-código)

---

## Visão Geral

Este projeto implementa um montador completo e simulador para a linguagem assembly inventada da disciplina de Software Básico. O sistema é dividido em três componentes principais:

1. **Pré-processador**: Processa diretivas EQU e IF, remove comentários e normaliza o código
2. **Montador**: Implementa algoritmo de duas passagens com resolução de pendências
3. **Simulador**: Executa o código objeto gerado, simulando a arquitetura proposta

---

## Requisitos do Sistema

### Sistema Operacional
- **Linux**: Ubuntu 20.04 ou superior (recomendado)
- **Windows**: Windows 10/11 com WSL2 ou MinGW
- **macOS**: macOS 10.15 ou superior (tratado como Linux)

### Compilador
- **GCC/G++**: Versão 7.0 ou superior
- **Padrão C++**: C++17 ou superior
- **Bibliotecas**: Apenas STL (Standard Template Library)

### Verificar Instalação
```bash
g++ --version
```

---

## Compilação

### Compilação Básica
```bash
g++ -std=c++17 -o montador montador.cpp
```

### Compilação com Avisos (Recomendado para Desenvolvimento)
```bash
g++ -std=c++17 -Wall -Wextra -o montador montador.cpp
```

### Compilação Otimizada (Para Produção)
```bash
g++ -std=c++17 -O2 -o montador montador.cpp
```

### Compilação com Debug
```bash
g++ -std=c++17 -g -o montador montador.cpp
```

---

## Modos de Operação

O programa detecta automaticamente o modo de operação pela extensão do arquivo de entrada.

### 1. Pré-processamento (.asm → .pre)

```bash
./montador arquivo.asm
```

**Funcionalidades:**
- Remove todos os comentários (iniciados por ponto-e-vírgula)
- Converte todo o código para maiúsculas (case insensitive)
- Resolve diretivas EQU (substitui símbolos por valores)
- Processa diretivas IF (compilação condicional)
- Reordena seções: SECTION TEXT sempre primeiro, SECTION DATA por último
- Aceita rótulos seguidos de dois-pontos e quebra de linha

**Entrada:** exemplo.asm
```assembly
SECTION TEXT
INPUT OLD_DATA
LOAD OLD_DATA
L1: DIV DOIS
STOP

SECTION DATA
DOIS: CONST 2
OLD_DATA: SPACE
```

**Saída:** exemplo.pre (normalizado e processado)

---

### 2. Montagem (.pre → .obj + .pen)

```bash
./montador arquivo.pre
```

**Funcionalidades:**
- **Passagem 1**: Constrói tabela de símbolos e calcula endereços
- **Passagem 2**: Gera código objeto e resolve pendências (backpatching)
- Gera dois arquivos de saída:
  - .obj: Código objeto completo (pronto para execução)
  - .pen: Código com pendências marcadas como -1 (simulação de passagem única)

**Algoritmo:**
- Implementa montador de duas passagens
- Suporta forward references (referências antecipadas)
- Resolve pendências através de backpatching

**Saída:**
- exemplo.obj: Código objeto completo
- exemplo.pen: Código com pendências marcadas como -1

---

### 3. Simulação (.obj)

```bash
./montador arquivo.obj
```

**Funcionalidades:**
- Carrega o código objeto na memória
- Simula a execução instrução por instrução
- Implementa todas as 14 instruções da arquitetura
- Interage com o usuário através de INPUT e OUTPUT

**Exemplo de Execução:**
```
--- INICIANDO SIMULAÇÃO ---
INPUT (endereço 29): 10
OUTPUT: 0
OUTPUT: 1
OUTPUT: 0
OUTPUT: 1
--- EXECUÇÃO ENCERRADA (STOP) ---
```

---

## Arquitetura do Montador

### Componentes Principais

```
montador.cpp
├── Utilitários (linhas 17-94)
│   ├── toUpper()      - Conversão case-insensitive
│   ├── trim()         - Remove espaços e tabulações
│   ├── tokenize()     - Tokenização (vírgulas → espaços)
│   ├── preTokenize()  - Tokenização (mantém vírgulas)
│   ├── strToInt()     - Conversão string→int (suporta hexadecimal)
│   ├── isNumber()     - Valida números
│   └── isValidLabel() - Valida rótulos
│
├── Preprocessor (linhas 123-237)
│   ├── Processa EQU e IF
│   ├── Remove comentários
│   └── Reordena seções
│
├── Assembler (linhas 242-512)
│   ├── pass1()        - Constrói tabela de símbolos
│   ├── pass2()        - Gera código objeto
│   └── generatePen()  - Simula passagem única
│
├── Simulator (linhas 517-576)
│   └── run()          - Executa código objeto
│
└── main() (linhas 581-620)
    └── Detecta modo e executa
```

---

## Especificação Técnica

### Instruções Suportadas (14 total)

| Opcode | Instrução | Operandos | Descrição |
|--------|-----------|-----------|-----------|
| 1 | ADD | 1 | ACC ← ACC + mem[OP] |
| 2 | SUB | 1 | ACC ← ACC - mem[OP] |
| 3 | MULT | 1 | ACC ← ACC * mem[OP] |
| 4 | DIV | 1 | ACC ← ACC / mem[OP] |
| 5 | JMP | 1 | PC ← OP |
| 6 | JMPN | 1 | if ACC < 0: PC ← OP |
| 7 | JMPP | 1 | if ACC > 0: PC ← OP |
| 8 | JZ | 1 | if ACC = 0: PC ← OP |
| 9 | COPY | 2 | mem[OP2] ← mem[OP1] |
| 10 | LOAD | 1 | ACC ← mem[OP] |
| 11 | STORE | 1 | mem[OP] ← ACC |
| 12 | INPUT | 1 | mem[OP] ← input() |
| 13 | OUTPUT | 1 | output(mem[OP]) |
| 14 | STOP | 0 | Encerra execução |

### Diretivas Suportadas

| Diretiva | Descrição | Exemplo |
|----------|-----------|---------|
| CONST | Define constante | DOIS: CONST 2 |
| SPACE | Reserva espaço | VETOR: SPACE 10 |
| EQU | Define símbolo | TAM EQU 100 |
| IF | Compilação condicional | IF DEBUG |
| SECTION TEXT | Início seção de código | - |
| SECTION DATA | Início seção de dados | - |

### Regras de Sintaxe

1. **Case Insensitive**: ADD, add, Add são equivalentes
2. **Comentários**: Iniciados por ponto-e-vírgula até o fim da linha
3. **Rótulos**: 
   - Formato: LABEL: ou LABEL: INSTRUCAO
   - Podem estar em linha separada
   - Devem começar com letra, seguido de letras, números ou underscore
4. **COPY**: Operandos separados por vírgula sem espaço: COPY A,B
5. **CONST**: Aceita decimal, negativo e hexadecimal (0x): CONST 0xFF
6. **Espaços**: Ignorados (exceto na vírgula do COPY)

### Validações Implementadas

- Rótulos não podem usar palavras reservadas
- Instruções só na SECTION TEXT
- CONST e SPACE só na SECTION DATA
- Número correto de operandos por instrução
- Símbolos devem ser definidos antes do uso (ou marcados como pendência)
- Proteção contra divisão por zero
- Proteção contra acesso inválido à memória
- Detecção de rótulos redefinidos

---

## Exemplos de Uso

### Exemplo 1: Programa Completo

**Arquivo:** exemplo.asm
```assembly
SECTION TEXT
INPUT OLD_DATA          ; Lê número do usuário
LOAD OLD_DATA
L1: DIV DOIS           ; Divide por 2
    STORE NEW_DATA
    MULT DOIS          ; Multiplica por 2
    STORE TMP_DATA
    LOAD OLD_DATA
    SUB TMP_DATA       ; Calcula resto
    STORE TMP_DATA
    OUTPUT TMP_DATA    ; Exibe resto (bit)
    COPY NEW_DATA,OLD_DATA
    LOAD OLD_DATA
    JMPP L1            ; Continua se > 0
    STOP

SECTION DATA
DOIS: CONST 2
OLD_DATA: SPACE
NEW_DATA: SPACE
TMP_DATA: SPACE
```

**Execução:**
```bash
# 1. Pré-processar
./montador exemplo.asm

# 2. Montar
./montador exemplo.pre

# 3. Executar
./montador exemplo.obj
```

### Exemplo 2: Usando EQU e IF

```assembly
DEBUG EQU 1
TAM EQU 10

SECTION TEXT
IF DEBUG
    OUTPUT MSG
LOAD VETOR
STOP

SECTION DATA
MSG: CONST 999
VETOR: SPACE TAM
```

---

## Tratamento de Erros

### Erros de Pré-processamento
- Nome inválido para EQU
- Valor inválido em EQU
- EQU redefinido
- IF requer exatamente um argumento
- Símbolo não definido para IF
- Código fora de SECTION

### Erros de Montagem
- Rótulo inválido
- Rótulo usa palavra reservada
- Rótulo redefinido
- Diretiva X fora da seção DATA
- Instrução X fora da seção TEXT
- CONST espera 1 argumento
- Valor inválido em SPACE
- Número incorreto de operandos
- COPY requer vírgula entre os operandos
- Símbolo não definido
- Operando imediato inválido

### Erros de Simulação
- Arquivo objeto não encontrado
- Acesso inválido à memória
- Divisão por zero
- Opcode inválido

---

## Estrutura do Código

### Classes Principais

#### 1. Preprocessor
```cpp
class Preprocessor {
public:
    void run(const string& inputFile, const string& outputFile);
};
```
- Processa arquivo .asm → .pre
- Resolve EQU e IF
- Reordena seções

#### 2. Assembler
```cpp
class Assembler {
private:
    void pass1(istream& in);
    void pass2();
    void generatePen(const string& penFile);
public:
    void assemble(const string& preFile, 
                  const string& objFile, 
                  const string& penFile);
};
```
- Implementa montador de duas passagens
- Gera .obj e .pen

#### 3. Simulator
```cpp
class Simulator {
public:
    void run(const string& objFile);
};
```
- Executa código objeto
- Simula todas as instruções

---

## Detalhes de Implementação

### Tabela de Símbolos
```cpp
map<string, pair<int, bool>> symTable;
// pair.first  = endereço
// pair.second = true se definido, false se pendência
```

### Lista de Pendências
```cpp
vector<pair<int, string>> pendencies;
// pair.first  = posição no código objeto
// pair.second = símbolo não resolvido
```

### Algoritmo de Backpatching
1. Na passagem 1, marca símbolos não definidos como pendências
2. Na passagem 2, coloca -1 onde há pendências
3. Após gerar todo o código, resolve todas as pendências
4. Se alguma pendência não for resolvida, gera erro

---

## Observações Importantes

1. **Sem Bibliotecas Externas**: Usa apenas STL (Standard Template Library)
2. **Portabilidade**: Código compatível com Linux, Windows (WSL/MinGW) e macOS
3. **Padrão C++17**: Requer compilador com suporte a C++17
4. **Arquivo .pen**: Representa o resultado da passagem única (com pendências)
5. **Arquivo .obj**: Código completo e pronto para execução
6. **Deslocamento**: Suporta notação LABEL+offset (exemplo: VETOR+5)

---

## Conformidade com a Especificação

Este projeto atende 100% dos requisitos da especificação do TP1:

- Montador chamado "montador"
- Três modos de execução (.asm, .pre, .obj)
- Case insensitive
- Aceita espaços e tabulações
- Aceita quebra de linha entre rótulo e operação
- COPY com vírgula sem espaço
- Comentários com ponto-e-vírgula
- Seções TEXT e DATA (reordenadas no pré-processamento)
- CONST aceita positivos, negativos e hexadecimal (0x)
- Diretivas EQU e IF
- Arquivo .obj (código objeto)
- Arquivo .pen (com pendências marcadas como -1)
- Simulador funcional
- INPUT do teclado, OUTPUT no monitor

---

**Universidade de Brasília (UnB)**  
**Disciplina:** Software Básico  
**Professor:** Bruno Macchiavello