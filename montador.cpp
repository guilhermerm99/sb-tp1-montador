#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <set>

using namespace std;

// ----------------------------------------------------------------------
// Utilitários
// ----------------------------------------------------------------------
string toUpper(const string& s) {
    string r = s;
    transform(r.begin(), r.end(), r.begin(), ::toupper);
    return r;
}

string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

// Tokenizador usado no montador (vírgulas viram espaços)
vector<string> tokenize(const string& line) {
    vector<string> tokens;
    string tmp = line;
    replace(tmp.begin(), tmp.end(), ',', ' ');
    stringstream ss(tmp);
    string tok;
    while (ss >> tok) tokens.push_back(tok);
    return tokens;
}

// Tokenizador especial para o pré-processador (mantém vírgulas)
vector<string> preTokenize(const string& line) {
    vector<string> tokens;
    string buf;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == ',') {
            if (!buf.empty()) {
                tokens.push_back(buf);
                buf.clear();
            }
            tokens.push_back(",");
        } else if (isspace(c)) {
            if (!buf.empty()) {
                tokens.push_back(buf);
                buf.clear();
            }
        } else {
            buf += c;
        }
    }
    if (!buf.empty()) tokens.push_back(buf);
    return tokens;
}

int strToInt(const string& s) {
    string val = trim(s);
    if (val.empty()) throw runtime_error("Valor numérico vazio");
    if (val.size() > 2 && (val[0] == '0' && (val[1] == 'X' || val[1] == 'x')))
        return stoi(val, nullptr, 16);
    return stoi(val);
}

bool isNumber(const string& s) {
    string v = trim(s);
    if (v.empty()) return false;
    if (v.size() > 2 && (v[0] == '0' && (v[1] == 'X' || v[1] == 'x'))) {
        for (size_t i = 2; i < v.size(); ++i)
            if (!isxdigit(v[i])) return false;
        return true;
    }
    size_t start = (v[0] == '+' || v[0] == '-') ? 1 : 0;
    if (start >= v.size()) return false;
    for (size_t i = start; i < v.size(); ++i)
        if (!isdigit(v[i])) return false;
    return true;
}

bool isValidLabel(const string& s) {
    if (s.empty() || !isalpha(s[0])) return false;
    for (char c : s)
        if (!isalnum(c) && c != '_') return false;
    return true;
}

// ----------------------------------------------------------------------
// Tabelas
// ----------------------------------------------------------------------
const set<string> instructions = {"ADD", "SUB", "MULT", "DIV", "JMP", "JMPN", "JMPP", "JZ",
                                  "COPY", "LOAD", "STORE", "INPUT", "OUTPUT", "STOP"};
const set<string> directives = {"CONST", "SPACE", "EQU", "IF", "SECTION", "EXTERN", "PUBLIC"};

const map<string, int> opcodeTable = {
    {"ADD", 1}, {"SUB", 2}, {"MULT", 3}, {"DIV", 4},
    {"JMP", 5}, {"JMPN", 6}, {"JMPP", 7}, {"JZ", 8},
    {"COPY", 9}, {"LOAD", 10}, {"STORE", 11},
    {"INPUT", 12}, {"OUTPUT", 13}, {"STOP", 14}
};

int numOperands(const string& instr) {
    if (instr == "COPY") return 2;
    if (instr == "STOP") return 0;
    return 1;
}

bool expectsAddress(const string& instr) {
    return numOperands(instr) > 0;
}

// ----------------------------------------------------------------------
// PRÉ‑PROCESSADOR
// ----------------------------------------------------------------------
class Preprocessor {
public:
    void run(const string& inputFile, const string& outputFile) {
        ifstream in(inputFile);
        ofstream out(outputFile);
        if (!in || !out) throw runtime_error("Erro ao abrir arquivos do pré-processador.");

        map<string, int> equTable;
        vector<string> textLines, dataLines;
        string section;
        string pendingLabel;
        bool ignoreNext = false;

        string line;
        while (getline(in, line)) {
            size_t com = line.find(';');
            if (com != string::npos) line = line.substr(0, com);
            line = trim(line);
            if (line.empty()) continue;

            line = toUpper(line);

            if (!pendingLabel.empty()) {
                line = pendingLabel + " " + line;
                pendingLabel.clear();
            }

            if (line.back() == ':') {
                stringstream ss(line);
                string lbl;
                ss >> lbl;
                string rest;
                if (!(ss >> rest)) {
                    pendingLabel = lbl + " ";
                    continue;
                }
            }

            // EQU
            if (line.find(" EQU ") != string::npos) {
                size_t pos = line.find(" EQU ");
                string name = trim(line.substr(0, pos));
                if (name.back() == ':') name = name.substr(0, name.size()-1);
                if (!isValidLabel(name)) throw runtime_error("Nome inválido para EQU: " + name);
                string valStr = trim(line.substr(pos + 5));
                if (!isNumber(valStr)) throw runtime_error("Valor inválido em EQU: " + line);
                if (equTable.count(name)) throw runtime_error("EQU redefinido: " + name);
                equTable[name] = strToInt(valStr);
                continue;
            }

            // IF
            vector<string> ptokens = preTokenize(line);
            if (!ptokens.empty() && ptokens[0] == "IF") {
                if (ptokens.size() != 2) throw runtime_error("IF requer exatamente um argumento");
                string token = ptokens[1];
                if (isNumber(token)) {
                    if (strToInt(token) == 0) ignoreNext = true;
                } else if (equTable.count(token)) {
                    if (equTable[token] == 0) ignoreNext = true;
                } else {
                    throw runtime_error("Símbolo não definido para IF: " + token);
                }
                continue;
            }

            if (ignoreNext) {
                ignoreNext = false;
                continue;
            }

            // Substituição de EQU (mantendo vírgulas)
            string processedLine;
            for (const string& tok : ptokens) {
                if (tok == ",") {
                    processedLine += ",";
                    continue;
                }

                string base = tok;
                string offStr;
                size_t plus = tok.find('+');
                if (plus != string::npos) {
                    base = tok.substr(0, plus);
                    offStr = tok.substr(plus);
                }

                if (equTable.count(base) && base != "EQU" && base != "IF") {
                    if (!processedLine.empty() && processedLine.back() != ',')
                        processedLine += " ";
                    processedLine += to_string(equTable[base]) + offStr;
                } else {
                    if (!processedLine.empty() && processedLine.back() != ',')
                        processedLine += " ";
                    processedLine += tok;
                }
            }

            if (processedLine == "SECTION TEXT") { section = "TEXT"; continue; }
            if (processedLine == "SECTION DATA") { section = "DATA"; continue; }

            if (section == "TEXT") textLines.push_back(processedLine);
            else if (section == "DATA") dataLines.push_back(processedLine);
            else throw runtime_error("Código fora de SECTION: " + processedLine);
        }

        out << "SECTION TEXT\n";
        for (const auto& l : textLines) out << l << "\n";
        if (!dataLines.empty()) {
            out << "SECTION DATA\n";
            for (const auto& l : dataLines) out << l << "\n";
        }
        cout << "[PRE] Arquivo pré-processado: " << outputFile << endl;
    }
};

// ----------------------------------------------------------------------
// MONTADOR (duas passagens)
// ----------------------------------------------------------------------
class Assembler {
private:
    struct LineInfo {
        string label;
        string instruction;
        vector<string> operands;
    };

    vector<LineInfo> parsedLines;
    map<string, pair<int, bool>> symTable;
    int locationCounter;
    vector<int> objectCode;
    vector<pair<int, string>> pendencies;

    void reset() {
        parsedLines.clear();
        symTable.clear();
        objectCode.clear();
        pendencies.clear();
        locationCounter = 0;
    }

    void parseLine(const string& line) {
        LineInfo info;
        string temp = trim(line);
        if (temp.empty()) return;

        // Rótulo
        if (temp.find(':') != string::npos) {
            size_t colon = temp.find(':');
            info.label = trim(temp.substr(0, colon));
            if (!isValidLabel(info.label)) throw runtime_error("Rótulo inválido: " + info.label);
            if (instructions.count(info.label) || directives.count(info.label) || info.label == "TEXT" || info.label == "DATA")
                throw runtime_error("Rótulo usa palavra reservada: " + info.label);
            temp = trim(temp.substr(colon + 1));
        }

        if (temp.empty()) {
            parsedLines.push_back(info);
            return;
        }

        vector<string> tokens = tokenize(temp);
        if (tokens.empty()) return;

        info.instruction = tokens[0];
        for (size_t i = 1; i < tokens.size(); ++i)
            info.operands.push_back(tokens[i]);

        // Validação rigorosa do COPY: vírgula sem espaços
        if (info.instruction == "COPY") {
            size_t comma = temp.find(',');
            if (comma == string::npos)
                throw runtime_error("COPY requer vírgula entre os operandos");
            // Verifica se há caracteres de espaço imediatamente antes ou depois da vírgula
            if (comma > 0 && isspace(temp[comma-1]))
                throw runtime_error("COPY não deve ter espaço antes da vírgula");
            if (comma + 1 < temp.size() && isspace(temp[comma+1]))
                throw runtime_error("COPY não deve ter espaço depois da vírgula");
            // Verifica se há mais de uma vírgula
            if (temp.find(',', comma+1) != string::npos)
                throw runtime_error("COPY deve ter exatamente uma vírgula");
        }

        parsedLines.push_back(info);
    }

    void pass1(istream& in) {
        string line;
        string section;
        locationCounter = 0;

        while (getline(in, line)) {
            size_t com = line.find(';');
            if (com != string::npos) line = line.substr(0, com);
            line = trim(line);
            if (line.empty()) continue;

            line = toUpper(line);

            if (line == "SECTION TEXT") { section = "TEXT"; continue; }
            if (line == "SECTION DATA") { section = "DATA"; continue; }

            if (section.empty()) throw runtime_error("Código fora de SECTION: " + line);

            parseLine(line);
            const LineInfo& info = parsedLines.back();

            if (!info.label.empty()) {
                if (symTable.count(info.label) && symTable[info.label].second)
                    throw runtime_error("Rótulo redefinido: " + info.label);
                symTable[info.label] = {locationCounter, true};
            }

            if (info.instruction.empty()) continue;

            if (section == "TEXT" && (info.instruction == "CONST" || info.instruction == "SPACE"))
                throw runtime_error("Diretiva " + info.instruction + " fora da seção DATA");
            if (section == "DATA" && instructions.count(info.instruction))
                throw runtime_error("Instrução " + info.instruction + " fora da seção TEXT");

            string instr = info.instruction;
            if (instr == "CONST") {
                if (info.operands.size() != 1) throw runtime_error("CONST espera 1 argumento");
                locationCounter++;
            } else if (instr == "SPACE") {
                int size = 1;
                if (!info.operands.empty()) {
                    if (!isNumber(info.operands[0])) throw runtime_error("Valor inválido em SPACE");
                    size = strToInt(info.operands[0]);
                }
                locationCounter += size;
            } else if (opcodeTable.count(instr)) {
                int expected = numOperands(instr);
                if ((int)info.operands.size() != expected)
                    throw runtime_error("Número incorreto de operandos para " + instr);

                for (const string& op : info.operands) {
                    string label = op;
                    size_t plus = label.find('+');
                    if (plus != string::npos) label = trim(label.substr(0, plus));
                    if (!isNumber(label) && !symTable.count(label)) {
                        symTable[label] = {-1, false};
                    }
                }

                locationCounter += 1 + expected;
            } else {
                throw runtime_error("Instrução/diretiva desconhecida: " + instr);
            }
        }
    }

    void pass2() {
        int lc = 0;
        for (const auto& info : parsedLines) {
            if (info.instruction.empty()) continue;

            string instr = info.instruction;
            if (instr == "CONST") {
                objectCode.push_back(strToInt(info.operands[0]));
                lc++;
            } else if (instr == "SPACE") {
                int size = 1;
                if (!info.operands.empty()) size = strToInt(info.operands[0]);
                for (int i = 0; i < size; i++) objectCode.push_back(0);
                lc += size;
            } else if (opcodeTable.count(instr)) {
                objectCode.push_back(opcodeTable.at(instr));
                lc++;
                for (const string& op : info.operands) {
                    string label = op;
                    int offset = 0;
                    size_t plus = label.find('+');
                    if (plus != string::npos) {
                        string offStr = trim(label.substr(plus+1));
                        if (!isNumber(offStr)) throw runtime_error("Deslocamento inválido: " + op);
                        offset = strToInt(offStr);
                        label = trim(label.substr(0, plus));
                    }

                    if (isNumber(label)) {
                        if (expectsAddress(instr))
                            throw runtime_error("Operando imediato inválido para " + instr + ": " + label);
                        objectCode.push_back(strToInt(label));
                    } else {
                        if (!isValidLabel(label)) throw runtime_error("Símbolo inválido: " + label);
                        if (symTable.count(label)) {
                            if (symTable[label].second) {
                                objectCode.push_back(symTable[label].first + offset);
                            } else {
                                objectCode.push_back(-1);
                                pendencies.push_back({(int)objectCode.size()-1, label});
                            }
                        } else {
                            throw runtime_error("Símbolo não definido: " + label);
                        }
                    }
                    lc++;
                }
            }
        }

        // Backpatching (resolve pendências)
        for (auto& p : pendencies) {
            int pos = p.first;
            string sym = p.second;
            if (symTable.count(sym) && symTable[sym].second)
                objectCode[pos] = symTable[sym].first;
            else
                throw runtime_error("Pendência não resolvida: símbolo " + sym + " continua indefinido");
        }
    }

    // Simula passagem única para gerar o .pen
    void generatePen(const string& penFile) {
        vector<int> penCode;
        map<string, int> localSymTable;     // símbolos já definidos (endereço)
        vector<pair<int, string>> localPendencies;

        int lc = 0;
        for (const auto& info : parsedLines) {
            // Rótulo definido aqui
            if (!info.label.empty()) {
                localSymTable[info.label] = lc;
            }
            if (info.instruction.empty()) continue;

            string instr = info.instruction;
            if (instr == "CONST") {
                penCode.push_back(strToInt(info.operands[0]));
                lc++;
            } else if (instr == "SPACE") {
                int size = 1;
                if (!info.operands.empty()) size = strToInt(info.operands[0]);
                for (int i = 0; i < size; i++) penCode.push_back(0);
                lc += size;
            } else if (opcodeTable.count(instr)) {
                penCode.push_back(opcodeTable.at(instr));
                lc++;
                for (const string& op : info.operands) {
                    string label = op;
                    int offset = 0;
                    size_t plus = label.find('+');
                    if (plus != string::npos) {
                        string offStr = trim(label.substr(plus+1));
                        offset = strToInt(offStr);
                        label = trim(label.substr(0, plus));
                    }

                    if (isNumber(label)) {
                        penCode.push_back(strToInt(label));
                    } else {
                        // Símbolo: já foi definido até aqui?
                        if (localSymTable.count(label)) {
                            penCode.push_back(localSymTable[label] + offset);
                        } else {
                            // Pendência!
                            penCode.push_back(-1);
                            localPendencies.push_back({(int)penCode.size()-1, label});
                        }
                    }
                    lc++;
                }
            }
        }

        // Grava o .pen com -1 nos locais pendentes
        ofstream penOut(penFile);
        for (size_t i = 0; i < penCode.size(); ++i) {
            if (i > 0) penOut << " ";
            penOut << penCode[i];
        }
        penOut << "\n";  // newline final
        penOut.close();
    }

public:
    void assemble(const string& preFile, const string& objFile, const string& penFile) {
        reset();
        ifstream in(preFile);
        if (!in) throw runtime_error("Não foi possível abrir " + preFile);

        pass1(in);
        in.close();
        pass2();

        // .obj
        ofstream objOut(objFile);
        for (size_t i = 0; i < objectCode.size(); ++i) {
            if (i > 0) objOut << " ";
            objOut << objectCode[i];
        }
        objOut << "\n";  // newline final
        objOut.close();

        // .pen (simulação de passagem única)
        generatePen(penFile);

        cout << "[ASM] Montagem concluída: " << objFile << ", " << penFile << endl;
    }
};

// ----------------------------------------------------------------------
// SIMULADOR
// ----------------------------------------------------------------------
class Simulator {
public:
    void run(const string& objFile) {
        ifstream in(objFile);
        if (!in) throw runtime_error("Arquivo objeto não encontrado: " + objFile);

        vector<int> mem;
        int val;
        while (in >> val) mem.push_back(val);
        in.close();

        int pc = 0, acc = 0;
        cout << "\n--- INICIANDO SIMULAÇÃO ---\n";
        while (pc >= 0 && pc < (int)mem.size()) {
            int opcode = mem[pc++];
            switch (opcode) {
                case 1: { int a = mem[pc++]; if (a<0||a>=(int)mem.size()) throw runtime_error("Acesso inválido"); acc += mem[a]; break; }
                case 2: { int a = mem[pc++]; if (a<0||a>=(int)mem.size()) throw runtime_error("Acesso inválido"); acc -= mem[a]; break; }
                case 3: { int a = mem[pc++]; if (a<0||a>=(int)mem.size()) throw runtime_error("Acesso inválido"); acc *= mem[a]; break; }
                case 4: {
                    int a = mem[pc++];
                    if (a<0||a>=(int)mem.size()) throw runtime_error("Acesso inválido");
                    int divisor = mem[a];
                    if (divisor == 0) throw runtime_error("Divisão por zero");
                    acc /= divisor;
                    break;
                }
                case 5: pc = mem[pc]; break;
                case 6: if (acc < 0) pc = mem[pc]; else pc++; break;
                case 7: if (acc > 0) pc = mem[pc]; else pc++; break;
                case 8: if (acc == 0) pc = mem[pc]; else pc++; break;
                case 9: { // COPY fonte, destino
                    int src  = mem[pc++];
                    int dest = mem[pc++];
                    if (dest < 0 || dest >= (int)mem.size() || src < 0 || src >= (int)mem.size())
                        throw runtime_error("Acesso inválido (COPY)");
                    mem[dest] = mem[src];
                    break;
                }
                case 10: { int a = mem[pc++]; if (a<0||a>=(int)mem.size()) throw runtime_error("Acesso inválido"); acc = mem[a]; break; }
                case 11: { int a = mem[pc++]; if (a<0||a>=(int)mem.size()) throw runtime_error("Acesso inválido"); mem[a] = acc; break; }
                case 12: {
                    int a = mem[pc++];
                    if (a < 0 || a >= (int)mem.size()) throw runtime_error("Endereço INPUT inválido");
                    cout << "INPUT (endereço " << a << "): ";
                    cin >> mem[a];
                    break;
                }
                case 13: {
                    int a = mem[pc++];
                    if (a<0||a>=(int)mem.size()) throw runtime_error("Endereço OUTPUT inválido");
                    cout << "OUTPUT: " << mem[a] << endl;
                    break;
                }
                case 14: cout << "--- EXECUÇÃO ENCERRADA (STOP) ---\n"; return;
                default: throw runtime_error("Opcode inválido: " + to_string(opcode));
            }
        }
    }
};

// ----------------------------------------------------------------------
// MAIN
// ----------------------------------------------------------------------
int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Uso:\n"
             << "  ./montador arquivo.asm   -> Pré-processamento\n"
             << "  ./montador arquivo.pre   -> Montagem (.obj e .pen)\n"
             << "  ./montador arquivo.obj   -> Simulação\n";
        return 1;
    }

    string arg = argv[1];
    size_t dot = arg.find_last_of('.');
    if (dot == string::npos) {
        cerr << "Arquivo sem extensão válida.\n";
        return 1;
    }
    string base = arg.substr(0, dot);
    string ext  = arg.substr(dot);

    try {
        if (ext == ".asm") {
            Preprocessor pp;
            pp.run(arg, base + ".pre");
        }
        else if (ext == ".pre") {
            Assembler as;
            as.assemble(arg, base + ".obj", base + ".pen");
        }
        else if (ext == ".obj") {
            Simulator sim;
            sim.run(arg);
        }
        else {
            cerr << "Extensão não suportada. Use .asm, .pre ou .obj\n";
            return 1;
        }
    } catch (const exception& e) {
        cerr << "ERRO: " << e.what() << endl;
        return 1;
    }

    return 0;
}